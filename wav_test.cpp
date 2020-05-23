#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <memory>

#include <cstddef>

#include "synthizer.h"
#include "synthizer_constants.h"

#include "synthizer/audio_output.hpp"
#include "synthizer/byte_stream.hpp"
#include "synthizer/context.hpp"
#include "synthizer/decoding.hpp"
#include "synthizer/downsampler.hpp"
#include "synthizer/filter_design.hpp"
#include "synthizer/hrtf.hpp"
#include "synthizer/iir_filter.hpp"
#include "synthizer/invokable.hpp"
#include "synthizer/logging.hpp"
#include "synthizer/panner_bank.hpp"
#include "synthizer/math.hpp"
#include "synthizer/sources.hpp"

#include "synthizer/generators/decoding.hpp"

using namespace synthizer;

#define CHECKED(x) do { \
auto ret = x; \
	if (ret) { \
		printf("Synthizer error code %i message %s", ret, syz_getLastErrorMessage());\
		return 1; \
	} \
} while(0)

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("Usage: wav_test <path>\n");
		return 1;
	}

	CHECKED(syz_configureLoggingBackend(SYZ_LOGGING_BACKEND_STDERR, nullptr));
	CHECKED(syz_initialize());

	syz_Handle context, generator, source;
	CHECKED(syz_createContext(&context));
	CHECKED(syz_createPannedSource(&source, context));
	CHECKED(syz_createStreamingGenerator(&generator, context, "file", argv[1], ""));
	CHECKED(syz_sourceAddGenerator(source, generator));

	double angle = 0;
	double delta = (360.0/20)*0.02;
	//delta = 0;
	printf("angle delta %f\n", delta);

	for(;;) {
		CHECKED(syz_setD(source, SYZ_PANNED_SOURCE_AZIMUTH, angle));
		angle += delta;
		angle -= (int(angle)/360)*360;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	syz_shutdown();
	return 0;
}
