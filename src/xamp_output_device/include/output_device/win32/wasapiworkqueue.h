//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/win32/hrexception.h>
#include <output_device/win32/unknownimpl.h>

#include <base/assert.h>

namespace xamp::output_device::win32 {

template <typename ParentType>
class WasapiWorkQueue : public UnknownImpl<IMFAsyncCallback> {
public:
	typedef HRESULT(ParentType::* Callback)(IMFAsyncResult*);

	WasapiWorkQueue(const std::wstring &mmcss_name, ParentType* parent, const Callback fn)
		: mmcss_name_(mmcss_name)
		, queue_id_(MAXDWORD)
		, task_id_(0)
		, workitem_key_(0)
		, parent_(parent)
		, callback_(fn) {
		XAMP_ASSERT(parent != nullptr);
		XAMP_ASSERT(fn != nullptr);
	}

	~WasapiWorkQueue() override {
		Destroy();
	}

	bool IsValid() const noexcept {
		return queue_id_ != MAXDWORD;
	}

	void Destroy() {
		if (!IsValid()) {
			return;
		}
		async_result_.Release();
		HrIfFailledThrow(::MFUnlockWorkQueue(queue_id_));
		queue_id_ = MAXDWORD;
		task_id_ = 0;
	}

	STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override {
		if (!ppv) {
			return E_POINTER;
		}

		if (iid == __uuidof(IUnknown)) {
			*ppv = static_cast<IUnknown*>(static_cast<IMFAsyncCallback*>(this));
		}
		else if (iid == __uuidof(IMFAsyncCallback)) {
			*ppv = static_cast<IMFAsyncCallback*>(this);
		}
		else {
			*ppv = nullptr;
			return E_NOINTERFACE;
		}

		AddRef();
		return S_OK;
	}

	STDMETHODIMP GetParameters(DWORD* flags, DWORD* queue) override {
		*flags = 0;
		*queue = queue_id_;
		return S_OK;
	}

	void Initial() {
		DWORD queue_id = MF_MULTITHREADED_WORKQUEUE;
		HrIfFailledThrow(::MFLockSharedWorkQueue(mmcss_name_.c_str(), 0, &task_id_, &queue_id));
		queue_id_ = queue_id;
		HrIfFailledThrow(::MFCreateAsyncResult(nullptr, this, nullptr, &async_result_));
	}

	STDMETHODIMP Invoke(IMFAsyncResult* async_result) override {
		if (!IsValid()) {
			return S_OK;
		}
		return (parent_->*callback_)(async_result);
	}

	void WaitAsync(HANDLE event) {
		workitem_key_ = 0;
		HrIfFailledThrow(::MFPutWaitingWorkItem(event, 1, async_result_, &workitem_key_));
	}

private:
	std::wstring mmcss_name_;
	DWORD queue_id_;
	DWORD task_id_;
	MFWORKITEM_KEY workitem_key_;
	ParentType* parent_;
	CComPtr<IMFAsyncResult> async_result_;
	const Callback callback_; 
};

template <typename T>
CComPtr<WasapiWorkQueue<T>> MakeWasapiWorkQueue(const std::wstring& mmcss_name,
	T* ptr, typename WasapiWorkQueue<T>::Callback callback) {
	return CComPtr<WasapiWorkQueue<T>>(new WasapiWorkQueue<T>(mmcss_name,
		ptr,
		callback));
}

}
