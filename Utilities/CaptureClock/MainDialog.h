////////////////////////////////////////////////////////////
// Copyright (C) Roman Ryltsov, 2012
// Created by Roman Ryltsov roman@alax.info
// 
// $Id$

#pragma once

#include "rodshow.h"
#include "rowinhttp.h"
#include "rocrypt.h"
#include "AboutDialog.h"

////////////////////////////////////////////////////////////
// CFilterData

class CFilterData
{
public:
	CComPtr<IMoniker> m_pMoniker;
	CStringW m_sMonikerDisplayName;
	CComPtr<IPropertyBag> m_pPropertyBag;
	CStringW m_sFriendlyName;

	const CComPtr<IPropertyBag>& PropertyBagNeeded()
	{
		if(!m_pPropertyBag)
		{
			_A(m_pMoniker);
			__C(m_pMoniker->BindToStorage(NULL, NULL, __uuidof(IPropertyBag), (VOID**) &m_pPropertyBag));
			_A(m_pPropertyBag);
		}
		return m_pPropertyBag;
	}

public:
// CFilterData
	CFilterData() throw()
	{
	}
	CFilterData(IMoniker* pMoniker) throw() :
		m_pMoniker(pMoniker)
	{
	}
	const CComPtr<IMoniker>& GetMoniker() const throw()
	{
		return m_pMoniker;
	}
	CStringW GetMonikerDisplayName()
	{
		_A(m_pMoniker);
		if(m_sMonikerDisplayName.IsEmpty())
			m_sMonikerDisplayName = _FilterGraphHelper::GetMonikerDisplayName(m_pMoniker);
		return m_sMonikerDisplayName;
	}
	CStringW GetFriendlyName()
	{
		if(m_sFriendlyName.IsEmpty())
			m_sFriendlyName = _FilterGraphHelper::ReadPropertyBagString(PropertyBagNeeded(), L"FriendlyName");
		return m_sFriendlyName;
	}
	CComPtr<IBaseFilter> CreateBaseFilterInstance() const
	{
		_A(m_pMoniker);
		CComPtr<IBaseFilter> pBaseFilter;
		__C(m_pMoniker->BindToObject(NULL, NULL, __uuidof(IBaseFilter), (VOID**) &pBaseFilter));
		return pBaseFilter;
	}
};

////////////////////////////////////////////////////////////
// CFilterDataListT

template <typename _FilterData, const GUID* t_pCategory>
class CFilterDataListT :
	public CRoListT<_FilterData>
{
public:
	typedef _FilterData CFilterData;

public:
// CFilterDataListT
	VOID Initialize()
	{
		RemoveAll();
		CComPtr<ICreateDevEnum> pCreateDevEnum;
		__C(pCreateDevEnum.CoCreateInstance(CLSID_SystemDeviceEnum));
		CComPtr<IEnumMoniker> pEnumMoniker;
		__C(pCreateDevEnum->CreateClassEnumerator(*t_pCategory, &pEnumMoniker, 0));
		CComPtr<IMoniker> pMoniker;
		while(pEnumMoniker->Next(1, &pMoniker, NULL) == S_OK)
		{
			_W(AddTail(_FilterData(pMoniker)));
			pMoniker.Release();
		}
		#pragma region Duplicate Suffixes
		CRoMapT<CStringW, CRoArrayT<_FilterData*>, CStringElementTraitsI<CStringW>> FriendlyNameMap;
		for(POSITION Position = GetHeadPosition(); Position; GetNext(Position))
		{
			_FilterData& FilterData = GetAt(Position);
			const CStringW sFriendlyName = FilterData.GetFriendlyName();
			_W(FriendlyNameMap[sFriendlyName].Add(&FilterData) >= 0);
		}
		for(POSITION Position = FriendlyNameMap.GetStartPosition(); Position; FriendlyNameMap.GetNext(Position))
		{
			const CStringW& sFriendlyName = FriendlyNameMap.GetKeyAt(Position);
			CRoArrayT<_FilterData*>& Array = FriendlyNameMap.GetValueAt(Position);
			if(Array.GetCount() <= 1)
				continue;
			for(SIZE_T nIndex = 0; nIndex < Array.GetCount(); nIndex++)
				Array[nIndex]->m_sFriendlyName = AtlFormatStringT<CStringW>(L"%s #%d", sFriendlyName, nIndex + 1);
		}
		#pragma endregion 
	}
	POSITION LookupMonikerDisplayName(LPCWSTR pszMonikerDisplayName)
	{
		_A(pszMonikerDisplayName);
		for(POSITION Position = GetHeadPosition(); Position; GetNext(Position))
			if(GetAt(Position).GetMonikerDisplayName().CompareNoCase(pszMonikerDisplayName) == 0)
				return Position;
		return NULL;
	}
	POSITION LookupMonikerDisplayName(CAtlRegExp<CAtlRECharTraitsW>& Expression)
	{
		CAtlREMatchContext<CAtlRECharTraitsW> MatchContext;
		for(POSITION Position = GetHeadPosition(); Position; GetNext(Position))
			if(Expression.Match(GetAt(Position).GetMonikerDisplayName(), &MatchContext))
				return Position;
		return NULL;
	}
	POSITION LookupFriendlyName(LPCWSTR pszFriendlyName)
	{
		_A(pszFriendlyName);
		for(POSITION Position = GetHeadPosition(); Position; GetNext(Position))
			if(GetAt(Position).GetFriendlyName().Compare(pszFriendlyName) == 0)
				return Position;
		return NULL;
	}
	POSITION LookupFriendlyName(CAtlRegExp<CAtlRECharTraitsW>& Expression)
	{
		CAtlREMatchContext<CAtlRECharTraitsW> MatchContext;
		for(POSITION Position = GetHeadPosition(); Position; GetNext(Position))
			if(Expression.Match(GetAt(Position).GetFriendlyName(), &MatchContext))
				return Position;
		return NULL;
	}
	POSITION LookupGenericName(LPCWSTR pszGenericName)
	{
		_A(pszGenericName);
		POSITION Position;
		Position = LookupMonikerDisplayName(pszGenericName);
		if(Position)
			return Position;
		Position = LookupFriendlyName(pszGenericName);
		if(Position)
			return Position;
		CAtlRegExp<CAtlRECharTraitsW> Expression;
		if(Expression.Parse(pszGenericName, TRUE) == REPARSE_ERROR_OK)
		{
			Position = LookupMonikerDisplayName(Expression);
			if(Position)
				return Position;
			Position = LookupFriendlyName(Expression);
			if(Position)
				return Position;
		}
		return NULL;
	}
};

////////////////////////////////////////////////////////////
// CAudioCaptureSourceData, CAudioCaptureSourceDataList

typedef CFilterData CAudioCaptureSourceData;
typedef CFilterDataListT<CAudioCaptureSourceData, &CLSID_AudioInputDeviceCategory> CAudioCaptureSourceDataList;

////////////////////////////////////////////////////////////
// CVideoCaptureSourceData, CVideoCaptureSourceDataList

typedef CFilterData CVideoCaptureSourceData;
typedef CFilterDataListT<CVideoCaptureSourceData, &CLSID_VideoInputDeviceCategory> CVideoCaptureSourceDataList;

////////////////////////////////////////////////////////////
// CMainDialog

class CMainDialog : 
	public CAxDialogImpl<CMainDialog>
{
public:
	enum { IDD = IDD_MAIN };

BEGIN_MSG_MAP_EX(CMainDialog)
	CHAIN_MSG_MAP(CAxDialogImpl<CMainDialog>)
	MSG_WM_INITDIALOG(OnInitDialog)
	MSG_WM_DESTROY(OnDestroy)
	MSG_WM_TIMER(OnTimer)
	MSG_WM_SYSCOMMAND(OnSysCommand)
	COMMAND_ID_HANDLER_EX(IDCANCEL, OnCommand)
	COMMAND_HANDLER_EX(IDC_START, BN_CLICKED, OnStartClicked)
	COMMAND_HANDLER_EX(IDC_STOP, BN_CLICKED, OnStopClicked)
	REFLECT_NOTIFICATIONS()
END_MSG_MAP()

public:

	////////////////////////////////////////////////////////
	// Timer Identifiers

	enum 
	{
		TIMER_FIRST,
		TIMER_UPDATE
	};

	////////////////////////////////////////////////////////
	// CRendererFilter

	class ATL_NO_VTABLE CRendererFilter :
		public CComObjectRootEx<CComMultiThreadModelNoCS>,
		public CBaseFilterT<CRendererFilter>,
		public CBasePersistT<CRendererFilter>,
		public CAmFilterMiscFlagsT<CRendererFilter, AM_FILTER_MISC_FLAGS_IS_RENDERER>
	{
	public:
		//enum { IDR = IDR_RENDERERFILTER };

	DECLARE_NO_REGISTRY() //DECLARE_REGISTRY_RESOURCEID(IDR)

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	//DECLARE_QI_TRACE(CRendererFilter)

	BEGIN_COM_MAP(CRendererFilter)
		COM_INTERFACE_ENTRY(IBaseFilter)
		COM_INTERFACE_ENTRY(IMediaFilter)
		COM_INTERFACE_ENTRY_IID(__uuidof(IPersist), IBaseFilter)
		COM_INTERFACE_ENTRY(IAMFilterMiscFlags)
	END_COM_MAP()

		////////////////////////////////////////////////////////
		// CInputPin

		class ATL_NO_VTABLE CInputPin :
			public CComObjectRootEx<CComMultiThreadModelNoCS>,
			public CInputPinT<CInputPin, CRendererFilter>
		{
		public:

		//DECLARE_QI_TRACE(CRendererFilter::CInputPin)

		BEGIN_COM_MAP(CInputPin)
			COM_INTERFACE_ENTRY(IPin)
			COM_INTERFACE_ENTRY(IMemInputPin)
		END_COM_MAP()

		private:
			GUID m_MajorType;

		public:
		// CInputPin
			CInputPin() throw() :
				m_MajorType(MEDIATYPE_NULL)
			{
				_Z4(atlTraceRefcount, 4, _T("this 0x%p\n"), this);
			}
			~CInputPin() throw()
			{
				_Z4(atlTraceRefcount, 4, _T("this 0x%p\n"), this);
			}
			VOID EnumerateMediaTypes(CAtlList<CMediaType>& MediaTypeList) const
			{
				CRoCriticalSectionLock DataLock(GetDataCriticalSection());
				_A(m_MajorType != MEDIATYPE_NULL);
				AM_MEDIA_TYPE MediaType;
				ZeroMemory(&MediaType, sizeof MediaType);
				MediaType.majortype = m_MajorType;
				_W(MediaTypeList.AddTail(&MediaType));
			}
			BOOL CheckMediaType(const CMediaType& pMediaType) const throw()
			{
				_A(pMediaType);
				CRoCriticalSectionLock DataLock(GetDataCriticalSection());
				if(GetMediaTypeReference())
					return GetMediaTypeReference().Compare(pMediaType);
				return pMediaType->majortype == m_MajorType;
			}
			VOID SetMajorType(const GUID& MajorType) throw()
			{
				CRoCriticalSectionLock DataLock(GetDataCriticalSection());
				_A(!GetMediaTypeReference());
				m_MajorType = MajorType;
			}
		};

	private:
		CObjectPtr<CInputPin> m_pInputPin;
		SIZE_T m_nMediaSampleCount;
		REFERENCE_TIME m_nLastMediaSampleTime;

	public:
	// CRendererFilter
		CRendererFilter() throw() :
			CBasePersistT<CRendererFilter>(GetDataCriticalSection())
		{
			_Z4(atlTraceRefcount, 4, _T("this 0x%p\n"), this);
		}
		~CRendererFilter() throw()
		{
			_Z4(atlTraceRefcount, 4, _T("this 0x%p\n"), this);
		}
		HRESULT FinalConstruct() throw()
		{
			_ATLTRY
			{
				m_pInputPin.Construct()->Initialize(this, L"Input");
				AddPin(m_pInputPin);
			}
			_ATLCATCH(Exception)
			{
				_C(Exception);
			}
			return S_OK;
		}
		VOID FinalRelease()
		{
			m_pInputPin = NULL;
		}
		VOID CueFilter()
		{
			CRoCriticalSectionLock DataLock(GetDataCriticalSection());
			m_nMediaSampleCount = 0;
			m_nLastMediaSampleTime = 0;
		}
		VOID ReceiveMediaSample(IPin* pPin, IMediaSample2* pMediaSample, HRESULT& nReceiveResult)
		{
			_A(pPin && pMediaSample); 
			_A(nReceiveResult == S_OK);
			CMediaSampleProperties Properties(pMediaSample);
			CRoCriticalSectionLock DataLock(GetDataCriticalSection());
			m_nMediaSampleCount++;
			if(Properties.dwSampleFlags & AM_SAMPLE_TIMEVALID)
				m_nLastMediaSampleTime = Properties.tStart;
		}
		const CObjectPtr<CInputPin>& GetInputPin() const throw()
		{
			return m_pInputPin;
		}
		VOID Initialize(const GUID& MajorType)
		{
			GetInputPin()->SetMajorType(MajorType);
		}
		VOID GetData(SIZE_T& nMediaSampleCount, REFERENCE_TIME& nLastMediaSampleTime) const throw()
		{
			CRoCriticalSectionLock DataLock(GetDataCriticalSection());
			nMediaSampleCount = m_nMediaSampleCount;
			nLastMediaSampleTime = m_nLastMediaSampleTime;
		}
	};

private:
	CRoComboBoxT<CFilterData, CRoListControlDataTraitsT> m_VideoDeviceComboBox;
	CRoEdit m_VideoTimeEdit;
	CRoComboBoxT<CFilterData, CRoListControlDataTraitsT> m_AudioDeviceComboBox;
	CRoEdit m_AudioTimeEdit;
	CRoEdit m_SystemTimeEdit;
	CButton m_StartButton;
	CButton m_StopButton;
	CGenericFilterGraph m_FilterGraph;
	CObjectPtr<CRendererFilter> m_pVideoRendererFilter;
	CObjectPtr<CRendererFilter> m_pAudioRendererFilter;
	CComPtr<IReferenceClock> m_pReferenceClock;
	REFERENCE_TIME m_nAnchorTime;
	CString m_sLog;
	UINT m_nTimerEventIndex;

public:
// CMainDialog
	CMainDialog()
	{
	}
	VOID ReleaseFilterGraph()
	{
		if(m_FilterGraph.m_pMediaControl)
			_V(m_FilterGraph.m_pMediaControl->Stop());
		m_FilterGraph.Release();
		m_pVideoRendererFilter.Release();
		m_pAudioRendererFilter.Release();
		m_pReferenceClock.Release();
	}
	static CComPtr<IPin> GetCapturePin(IBaseFilter* pBaseFilter)
	{
		_A(pBaseFilter);
		_FilterGraphHelper::CPinArray PinArray;
		_FilterGraphHelper::GetFilterPins(pBaseFilter, PINDIR_OUTPUT, PinArray);
		CComPtr<IPin> pCapturePin, pAssumedCapturePin;
		for(SIZE_T nIndex = 0; nIndex < PinArray.GetCount(); nIndex++)
		{
			const CComPtr<IPin>& pPin = PinArray[nIndex];
			if(!pAssumedCapturePin)
			{
				pAssumedCapturePin = pPin;
				// SUGG: Check Media Type
			}
			const CComQIPtr<IKsPropertySet> pKsPropertySet = pPin;
			if(pKsPropertySet)
			{
				GUID PinCategory = GUID_NULL;
				DWORD nPinCategorySize = 0;
				const HRESULT nGetResult = pKsPropertySet->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, &PinCategory, sizeof PinCategory, &nPinCategorySize);
				_Z4(atlTraceGeneral, 4, _T("nGetResult 0x%08x, nPinCategorySize %d, PinCategory %ls\n"), nGetResult, nPinCategorySize, _PersistHelper::StringFromIdentifier(PinCategory));
				if(SUCCEEDED(nGetResult))
					if(nPinCategorySize == sizeof PinCategory && PinCategory == PIN_CATEGORY_CAPTURE)
					{
						pCapturePin = pPin;
						break;
					}
			}
		}
		return pCapturePin ? pCapturePin : pAssumedCapturePin;
	}
	VOID UpdateControls()
	{
		m_VideoDeviceComboBox.EnableWindow(m_FilterGraph.m_pFilterGraph == NULL);
		m_VideoDeviceComboBox.GetWindow(GW_HWNDPREV).EnableWindow(m_VideoDeviceComboBox.IsWindowEnabled());
		m_AudioDeviceComboBox.EnableWindow(m_FilterGraph.m_pFilterGraph == NULL);
		m_AudioDeviceComboBox.GetWindow(GW_HWNDPREV).EnableWindow(m_AudioDeviceComboBox.IsWindowEnabled());
		m_StartButton.EnableWindow(m_FilterGraph.m_pFilterGraph == NULL);
		m_StopButton.EnableWindow(m_FilterGraph.m_pFilterGraph != NULL);
	}

// Window Message Handelrs
	LRESULT OnInitDialog(HWND, LPARAM)
	{
		SetIcon(AtlLoadIconImage(IDI_MODULE, LR_DEFAULTCOLOR, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON)), TRUE);
		SetIcon(AtlLoadIconImage(IDI_MODULE, LR_DEFAULTCOLOR, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON)), FALSE);
		CMenuHandle Menu = GetSystemMenu(FALSE);
		_W(Menu.AppendMenu(MF_SEPARATOR));
		_W(Menu.AppendMenu(MF_STRING, ID_APP_ABOUT, _T("&About...")));
		m_VideoDeviceComboBox = GetDlgItem(IDC_VIDEO_DEVICE);
		CVideoCaptureSourceDataList VideoCaptureSourceDataList;
		VideoCaptureSourceDataList.Initialize();
		for(POSITION Position = VideoCaptureSourceDataList.GetHeadPosition(); Position; VideoCaptureSourceDataList.GetNext(Position))
		{
			CFilterData& FilterData = VideoCaptureSourceDataList.GetAt(Position);
			m_VideoDeviceComboBox.AddString(CW2CT(FilterData.GetFriendlyName()), FilterData);
		}
		m_VideoDeviceComboBox.SetCurSel(0);
		m_VideoTimeEdit = GetDlgItem(IDC_VIDEO_TIME);
		m_AudioDeviceComboBox = GetDlgItem(IDC_AUDIO_DEVICE);
		CAudioCaptureSourceDataList AudioCaptureSourceDataList;
		AudioCaptureSourceDataList.Initialize();
		for(POSITION Position = AudioCaptureSourceDataList.GetHeadPosition(); Position; AudioCaptureSourceDataList.GetNext(Position))
		{
			CFilterData& FilterData = AudioCaptureSourceDataList.GetAt(Position);
			m_AudioDeviceComboBox.AddString(CW2CT(FilterData.GetFriendlyName()), FilterData);
		}
		m_AudioDeviceComboBox.SetCurSel(0);
		m_AudioTimeEdit = GetDlgItem(IDC_AUDIO_TIME);
		m_SystemTimeEdit = GetDlgItem(IDC_SYSTEM_TIME);
		m_StartButton = GetDlgItem(IDC_START);
		m_StopButton = GetDlgItem(IDC_STOP);
		m_StopButton.EnableWindow(FALSE);
		_W(CenterWindow());
		UpdateControls();
		#if _DEVELOPMENT
		// TODO: ...
		#endif // _DEVELOPMENT
		return TRUE;
	}
	LRESULT OnDestroy() throw()
	{
		CWaitCursor WaitCursor;
		ReleaseFilterGraph();
		return 0;
	}
	LRESULT OnTimer(UINT_PTR nEvent)
	{
		switch(nEvent)
		{
		case TIMER_UPDATE:
			{
				// NOTE: A log item every half a minute
				const BOOL bLog = ++m_nTimerEventIndex % 30 == 0;
				CRoArrayT<CString> Array;
				REFERENCE_TIME nSystemTime;
				if(m_pReferenceClock)
				{
					_V(m_pReferenceClock->GetTime(&nSystemTime));
					nSystemTime -= m_nAnchorTime;
					m_SystemTimeEdit.SetValue(AtlFormatString(_T("%s"), _FilterGraphHelper::FormatReferenceTime(nSystemTime)));
					if(bLog)
						Array.Add(AtlFormatString(_T("%I64d"), (nSystemTime + 5000i64 - 1) / 10000i64));
				}
				if(m_pVideoRendererFilter)
				{
					SIZE_T nCount;
					REFERENCE_TIME nTime;
					m_pVideoRendererFilter->GetData(nCount, nTime);
					m_VideoTimeEdit.SetValue(AtlFormatString(_T("%s (%s samples)"), _FilterGraphHelper::FormatReferenceTime(nTime), _StringHelper::FormatNumber((INT) nCount)));
					if(bLog)
					{
						Array.Add(AtlFormatString(_T("%d"), nCount));
						Array.Add(AtlFormatString(_T("%I64d"), (nTime + 5000i64 - 1) / 10000i64));
						Array.Add(AtlFormatString(_T("%I64d"), ((nTime - nSystemTime) + 5000i64 - 1) / 10000i64));
					}
				}
				if(m_pAudioRendererFilter)
				{
					SIZE_T nCount;
					REFERENCE_TIME nTime;
					m_pAudioRendererFilter->GetData(nCount, nTime);
					m_AudioTimeEdit.SetValue(AtlFormatString(_T("%s (%s samples)"), _FilterGraphHelper::FormatReferenceTime(nTime), _StringHelper::FormatNumber((INT) nCount)));
					if(bLog)
					{
						Array.Add(AtlFormatString(_T("%d"), nCount));
						Array.Add(AtlFormatString(_T("%I64d"), (nTime + 5000i64 - 1) / 10000i64));
						Array.Add(AtlFormatString(_T("%I64d"), ((nTime - nSystemTime) + 5000i64 - 1) / 10000i64));
					}
				}
				if(bLog && Array.GetCount() == 1 + 3 + 3)
				{
					if(m_sLog.IsEmpty())
					{
						static LPCTSTR g_ppszHeader[] = 
						{
							_T("System Time"),
							_T("Video Sample Count"),
							_T("Video Sample Time"),
							_T("Relative Video Sample Time"),
							_T("Audio Sample Count"),
							_T("Audio Sample Time"),
							_T("Relative Audio Sample Time"),
						};
						m_sLog.Append(_StringHelper::Join(g_ppszHeader, _T("\t")) + _T("\r\n"));
					}
					m_sLog.Append(_StringHelper::Join(Array, _T("\t")) + _T("\r\n"));
				}
			}
			break;
		default:
			SetMsgHandled(FALSE);
		}
		return 0;
	}
	LRESULT OnSysCommand(UINT nCommand, CPoint)
	{
		switch(nCommand)
		{
		case ID_APP_ABOUT:
			{
				CAboutDialog Dialog;
				Dialog.DoModal(m_hWnd);
			}
			break;
		default:
			SetMsgHandled(FALSE);
		}
		return 0;
	}
	LRESULT OnCommand(UINT, INT nIdentifier, HWND)
	{
		_W(EndDialog(nIdentifier));
		return 0;
	}
	LRESULT OnStartClicked(UINT, INT, HWND)
	{
		CWaitCursor WaitCursor;
		m_StartButton.EnableWindow(FALSE);
		_ATLTRY
		{
			ReleaseFilterGraph();
			m_FilterGraph.CoCreateInstance();
			m_FilterGraph.SetShowDestructorMessageBox(TRUE);
			// SUGG: Video/Audio Only Capture?
			_A(m_VideoDeviceComboBox.GetCurSel() >= 0 && m_AudioDeviceComboBox.GetCurSel() >= 0);
			#pragma region Video
			{
				CFilterData& VideoFilterData = m_VideoDeviceComboBox.GetItemData(m_VideoDeviceComboBox.GetCurSel());
				const CComPtr<IBaseFilter> pSourceBaseFilter = VideoFilterData.CreateBaseFilterInstance();
				__C(m_FilterGraph->AddFilter(pSourceBaseFilter, CT2CW(_T("Video Source"))));
				CObjectPtr<CRendererFilter> pRendererFilter;
				pRendererFilter.Construct();
				pRendererFilter->Initialize(MEDIATYPE_Video);
				__C(m_FilterGraph->AddFilter(pRendererFilter, CT2CW(_T("Video Renderer"))));
				__C(m_FilterGraph->Connect(GetCapturePin(pSourceBaseFilter), pRendererFilter->GetInputPin()));
				m_pVideoRendererFilter = pRendererFilter;
			}
			#pragma endregion 
			#pragma region Audio
			{
				CFilterData& AudioFilterData = m_AudioDeviceComboBox.GetItemData(m_AudioDeviceComboBox.GetCurSel());
				const CComPtr<IBaseFilter> pSourceBaseFilter = AudioFilterData.CreateBaseFilterInstance();
				__C(m_FilterGraph->AddFilter(pSourceBaseFilter, CT2CW(_T("Audio Source"))));
				CObjectPtr<CRendererFilter> pRendererFilter;
				pRendererFilter.Construct();
				pRendererFilter->Initialize(MEDIATYPE_Audio);
				__C(m_FilterGraph->AddFilter(pRendererFilter, CT2CW(_T("Audio Renderer"))));
				const CComPtr<IPin> pCapturePin = GetCapturePin(pSourceBaseFilter);
				_ATLTRY
				{
					const CComQIPtr<IAMBufferNegotiation> pAmBufferNegotiation = pCapturePin;
					__D(pAmBufferNegotiation, E_NOINTERFACE);
					const CComQIPtr<IAMStreamConfig> pAmStreamConfig = pCapturePin;
					__D(pAmStreamConfig, E_NOINTERFACE);
					CMediaType pMediaType;
					__C(pAmStreamConfig->GetFormat(&pMediaType));
					const CWaveFormatEx* pWaveFormatEx = pMediaType.GetWaveFormatEx();
					__D(pWaveFormatEx, E_UNNAMED);
					ALLOCATOR_PROPERTIES Properties;
					Properties.cbAlign = -1;
					Properties.cbBuffer = pWaveFormatEx->nAvgBytesPerSec / 10; // 100 millisecond
					Properties.cbPrefix = -1;
					Properties.cBuffers = 50; // 50 buffers (5 seconds in total)
					__C(pAmBufferNegotiation->SuggestAllocatorProperties(&Properties));
				}
				_ATLCATCHALL()
				{
					_Z_EXCEPTION();
				}
				__C(m_FilterGraph->Connect(pCapturePin, pRendererFilter->GetInputPin()));
				m_pAudioRendererFilter = pRendererFilter;
			}
			#pragma endregion 
			m_FilterGraph.SetShowDestructorMessageBox(FALSE);
			__C(m_pReferenceClock.CoCreateInstance(CLSID_SystemClock));
			__C(m_pReferenceClock->GetTime(&m_nAnchorTime));
			__C(m_FilterGraph.m_pMediaControl->Run());
			m_sLog.Empty();
			m_nTimerEventIndex = 0;
			SetTimer(TIMER_UPDATE, 1000);
		}
		_ATLCATCHALL()
		{
			ReleaseFilterGraph();
			UpdateControls();
			_ATLRETHROW;
		}
		UpdateControls();
		return 0;
	}
	LRESULT OnStopClicked(UINT, INT, HWND)
	{
		CWaitCursor WaitCursor;
		m_StopButton.EnableWindow(FALSE);
		_ATLTRY
		{
			KillTimer(TIMER_UPDATE);
			ReleaseFilterGraph();
		}
		_ATLCATCHALL()
		{
			UpdateControls();
			_ATLRETHROW;
		}
		UpdateControls();
		#if !_DEVELOPMENT
			if(m_sLog.IsEmpty())
				return 0; // No Effective Log
		#endif // !_DEVELOPMENT
		#pragma region Log
		CString sLog;
		TCHAR pszComputerName[256] = { 0 };
		DWORD nComputerNameLength = DIM(pszComputerName);
		_W(GetComputerName(pszComputerName, &nComputerNameLength));
		sLog += AtlFormatString(_T("Computer Name") _T("\t") _T("%s") _T("\r\n"), pszComputerName);
		OSVERSIONINFO VersionInformation = { sizeof VersionInformation };
		_W(GetVersionEx(&VersionInformation));
		_A(VersionInformation.dwPlatformId == VER_PLATFORM_WIN32_NT);
		sLog += AtlFormatString(_T("Windows Version") _T("\t") _T("%d.%d.%d %s") _T("\r\n"), VersionInformation.dwMajorVersion, VersionInformation.dwMinorVersion, VersionInformation.dwBuildNumber, VersionInformation.szCSDVersion);
		CFilterData& VideoFilterData = m_VideoDeviceComboBox.GetItemData(m_VideoDeviceComboBox.GetCurSel());
		sLog += AtlFormatString(_T("Video Device") _T("\t") _T("%ls") _T("\t") _T("%ls") _T("\r\n"), VideoFilterData.GetFriendlyName(), VideoFilterData.GetMonikerDisplayName());
		CFilterData& AudioFilterData = m_AudioDeviceComboBox.GetItemData(m_AudioDeviceComboBox.GetCurSel());
		sLog += AtlFormatString(_T("Audio Device") _T("\t") _T("%ls") _T("\t") _T("%ls") _T("\r\n"), AudioFilterData.GetFriendlyName(), AudioFilterData.GetMonikerDisplayName());
		sLog += _T("\r\n");
		sLog += m_sLog;
		#pragma endregion
		SetClipboardText(m_hWnd, sLog);
		MessageBeep(MB_OK);
		#if !_DEVELOPMENT
			if(AtlMessageBoxEx(m_hWnd, 
					_T("The runtime execution data was placed onto clipboard in tab separated value format, you can paste it into a text or a spreadsheet document for further processing or analysis.") _T("\r\n")
					_T("\r\n")
					_T("Would you also like to share the results and post them to the website?"), IDS_CONFIRMATION, MB_ICONINFORMATION | MB_YESNO | ((m_nTimerEventIndex < 60) ? MB_DEFBUTTON2 : 0))  != IDYES)
				return 0; // No Submission
		#else
			return 0; // No Submission
		#endif // !_DEVELOPMENT
		#pragma region HTTP POST
		CWinHttpPostData PostData;
		PostData.AddValue(_T("s"), AtlLoadString(IDS_PROJNAME));
		CStringA sTextA = Utf8StringFromString(sLog);
		PostData.AddValue(_T("b"), _Base64Helper::Encode<CString>((const BYTE*) (LPCSTR) sTextA, sTextA.GetLength(), _Base64Helper::FLAG_NOPAD | _Base64Helper::FLAG_NOCRLF));
		CWinHttpSessionHandle Session = OpenWinHttpSessionHandle(AtlLoadString(IDS_PROJNAME));
		__E(Session);
		CWinHttpConnectionHandle Connection = Session.Connect(CStringW(_T("alax.info")));
		__E(Connection);
		CWinHttpRequestHandle Request = Connection.OpenRequest(CStringW(_T("POST")), CStringW(_T("/post.php")));
		__E(Request);
		CStringW sPostHeaders = PostData.GetHeaders();
		CStringA sPostData(PostData.GetValue());
		__E(Request.Send(sPostHeaders, -1, const_cast<LPSTR>((LPCSTR) sPostData), sPostData.GetLength(), sPostData.GetLength()));
		__E(Request.ReceiveResponse());
		DWORD nStatusCode = 0;
		__E(Request.QueryNumberHeader(WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nStatusCode));
		__D(nStatusCode == HTTP_STATUS_OK, MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, nStatusCode));
		#pragma endregion
		AtlMessageBoxEx(m_hWnd, _T("The results were posted to the website, thank you for sharing them!"), IDS_INFORMATION, MB_ICONINFORMATION | MB_OK);
		return 0;
	}
};