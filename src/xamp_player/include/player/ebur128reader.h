//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/player.h>

#include <base/pimplptr.h>
#include <base/stl.h>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN

class XAMP_PLAYER_API Ebur128Reader final {
public:
	Ebur128Reader();

	XAMP_PIMPL(Ebur128Reader)

	void SetSampleRate(uint32_t sample_rate);

	void Process(float const * samples, size_t num_sample);

	[[nodiscard]] double GetLoudness() const;

	[[nodiscard]] double GetTruePeek() const;

    [[nodiscard]] double GetSamplePeak() const;

	[[nodiscard]] void* GetNativeHandle() const;

	static double GetEbur128Gain(double lufs, double targetdb);

    static double GetMultipleLoudness(const Vector<Ebur128Reader>& scanners);

	static void LoadEbur128Lib();
private:
	class Ebur128ReaderImpl;
	AlignPtr<Ebur128ReaderImpl> impl_;
};

XAMP_AUDIO_PLAYER_NAMESPACE_END
