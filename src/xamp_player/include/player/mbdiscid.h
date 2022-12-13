//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/player.h>

#include <base/base.h>
#include <base/align_ptr.h>

#ifdef XAMP_OS_WIN

namespace xamp::player {

class XAMP_PLAYER_API MBDiscId {
public:
	MBDiscId();

	XAMP_PIMPL(MBDiscId)

	std::string GetDiscId(const std::string& drive) const;

	std::string GetFreeDBId(const std::string & drive) const;

	std::string GetSubmissionUrl(const std::string& drive) const;

	std::string GetDiscIdLookupUrl(const std::string& drive) const;

	static void LoadMBDiscIdLib();
private:
	class MBDiscIdImpl;
	AlignPtr<MBDiscIdImpl> impl_;
};

}

#endif
