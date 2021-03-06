#include "synthizer.h"

#include "synthizer/sources.hpp"

#include "synthizer/c_api.hpp"
#include "synthizer/config.hpp"
#include "synthizer/context.hpp"
#include "synthizer/generator.hpp"
#include "synthizer/math.hpp"
#include "synthizer/types.hpp"
#include "synthizer/vector_helpers.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

namespace synthizer {

void Source::addGenerator(std::shared_ptr<Generator> &generator) {
	if (this->hasGenerator(generator)) return;
	this->generators.emplace_back(generator);
}

void Source::removeGenerator(std::shared_ptr<Generator> &generator) {
	if (this->generators.empty()) return;
	if (this->hasGenerator(generator) == false) return;

	unsigned int index = 0;
	for(; index < this->generators.size(); index++) {
		auto s = this->generators[index].lock();
		if (s == generator) break;
	}

	std::swap(this->generators[this->generators.size()-1], this->generators[index]);
	this->generators.resize(this->generators.size() - 1);
}

bool Source::hasGenerator(std::shared_ptr<Generator> &generator) {
	return weak_vector::contains(this->generators, generator);
}

double Source::getGain() {
	return this->gain;
}

void Source::setGain(double gain) {
	this->gain = gain;
}

void Source::fillBlock(unsigned int channels) {
	alignas(config::ALIGNMENT) thread_local std::array<AudioSample, config::MAX_CHANNELS * config::BLOCK_SIZE> premix_array = {0.0f };
	AudioSample *premix = &premix_array[0];

	std::fill(&this->block[0], &this->block[0] + channels * config::BLOCK_SIZE, 0.0f);

	/* iterate and remove as we go to avoid locking weak_ptr twice. */
	weak_vector::iterate_removing(this->generators, [&] (auto &g) {
		unsigned int nch = g->getChannels();

		if (nch == 0) {
			return;
		}

		if (nch == channels) {
			g->generateBlock(&this->block[0]);
		} else {
			std::fill(premix, premix + config::BLOCK_SIZE * nch, 0.0f);
			g->generateBlock(premix);

			/*
			 * For now, we only support mono and stereo. This will eventually be moved
			 * to a remixing function.
			 * */
			if (channels == 1) {
				/* Downmix to mono. */
				float normfactor = 1.0f / nch;
				for (unsigned int c = 0; c < nch; c++) {
					for (unsigned int i = 0; i < config::BLOCK_SIZE; i++) {
						float s = premix[nch * i + c];
						block[i] += s * normfactor;
					}
				}
			} else if(nch == 1) {
				/* Broadcast to all channels. */
				for (unsigned int i = 0; i < config::BLOCK_SIZE; i++) {
					for(unsigned int j = 0; j < channels; j++)  {
						block[channels * i + j] += premix[i];
					}
				}
			} else {
				/*
				 * Take the lesser of the 2 channels. If this is input, then the extra output channels are untouched.
				 * If it's output, then the input channels are truncated.
				 * */
				auto final_ch = std::min(nch, channels);
				for (unsigned int i = 0; i < config::BLOCK_SIZE; i++) {
					for (unsigned int j = 0; j < final_ch; j++) {
						block[i * channels + j] += premix[i * nch + j];
					}
				}
			}
		}
	});

	float gainf = this->gain;
	for (unsigned int i = 0; i < config::BLOCK_SIZE * channels; i++) {
		this->block[i] *= gainf;
	}
}

}

#define PROPERTY_CLASS Source
#define PROPERTY_LIST SOURCE_PROPERTIES
#define PROPERTY_BASE BaseObject
#include "synthizer/property_impl.hpp"

using namespace synthizer;

SYZ_CAPI syz_ErrorCode syz_sourceAddGenerator(syz_Handle source, syz_Handle generator) {
	SYZ_PROLOGUE
	auto src = fromC<Source>(source);
	auto gen = fromC<Generator>(generator);
	src->getContextRaw()->call([&] () {
		src->addGenerator(gen);
	});
	return 0;
	SYZ_EPILOGUE
}

SYZ_CAPI syz_ErrorCode syz_sourceRemoveGenerator(syz_Handle source, syz_Handle generator) {
	SYZ_PROLOGUE
	auto src = fromC<Source>(source);
	auto gen = fromC<Generator>(generator);
	src->getContextRaw()->call([&] () {
		src->removeGenerator(gen);
	});
	return 0;
	SYZ_EPILOGUE
}
