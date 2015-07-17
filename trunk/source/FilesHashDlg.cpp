// MD5SUM03Dlg.cpp : 实现文件
//
#include "stdafx.h"

#include <string>

#include "strhelper.h"

#include "FilesHash.h"
#include "FilesHashDlg.h"
#include "FindDlg.h"
#include "AboutDlg.h"
#include "Global.h"
#include "Functions.h"
#include "HashEngine.h"
#include "UIStrings.h"
#include "UIBridgeMFC.h"


using namespace std;
using namespace sunjwbase;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CFilesHashDlg 对话框
CFilesHashDlg::CFilesHashDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFilesHashDlg::IDD, pParent),
	m_uiBridgeMFC(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
}

void CFilesHashDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS, m_prog);
	DDX_Control(pDX, IDC_PROG_WHOLE, m_progWhole);
	DDX_Control(pDX, IDE_TXTMAIN, m_editMain);
	DDX_Control(pDX, IDC_OPEN, m_btnOpen);
	DDX_Control(pDX, IDC_STOP, m_btnStop);
	DDX_Control(pDX, IDC_EXIT, m_btnExit);
	DDX_Control(pDX, IDC_CLEAN, m_btnClr);
	DDX_Control(pDX, IDC_COPY, m_btnCopy);
	DDX_Control(pDX, IDC_CHECKUP, m_chkUppercase);
	DDX_Control(pDX, IDC_FIND, m_btnFind);
	DDX_Control(pDX, IDC_CONTEXT, m_btnContext);
}

BEGIN_MESSAGE_MAP(CFilesHashDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_OPEN, OnBnClickedOpen)
	ON_BN_CLICKED(IDC_STOP, OnBnClickedStop)
	ON_BN_CLICKED(IDC_EXIT, OnBnClickedExit)
	ON_BN_CLICKED(IDC_ABOUT, OnBnClickedAbout)
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_CLEAN, OnBnClickedClean)
	ON_BN_CLICKED(IDC_COPY, OnBnClickedCopy)
	ON_WM_TIMER()
	ON_WM_CTLCOLOR()
	ON_WM_CLOSE()
	ON_MESSAGE(WM_THREAD_INFO, OnThreadMsg)
	ON_BN_CLICKED(IDC_FIND, &CFilesHashDlg::OnBnClickedFind)
	ON_BN_CLICKED(IDC_CONTEXT, &CFilesHashDlg::OnBnClickedContext)
	ON_BN_CLICKED(IDC_CHECKUP, &CFilesHashDlg::OnBnClickedCheckup)
END_MESSAGE_MAP()


// CFilesHashDlg 消息处理程序

BOOL CFilesHashDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO：在此添加额外的初始化代码

	m_bruEditBkg.CreateSolidBrush(RGB(255, 255, 255));

	PrepareAdvTaskbar();

	m_bFind = FALSE;
	m_btnClr.SetWindowText(MAINDLG_CLEAR);

	m_hWorkThread = NULL;
	m_waitingExit = FALSE;

	m_calculateTime = 0.0;
	m_timer = -1;

	m_prog.SetStep(1);

	m_bLimited = IsLimitedProc();

	CWnd* pWnd;
	pWnd = GetDlgItem(IDC_STATIC_SPEED);
	pWnd->SetWindowText(_T(""));
	pWnd = GetDlgItem(IDC_STATIC_TIME);
	pWnd->SetWindowText(_T(""));
	pWnd = GetDlgItem(IDC_STATIC_FILE);
	pWnd->SetWindowText(MAINDLG_FILE_PROGRESS);
	pWnd = GetDlgItem(IDC_STATIC_WHOLE);
	pWnd->SetWindowText(MAINDLG_TOTAL_PROGRESS);
	pWnd = GetDlgItem(IDC_STATIC_UPPER);
	pWnd->SetWindowText(MAINDLG_UPPER_HASH);
	pWnd = GetDlgItem(IDC_STATIC_UPPER);
	pWnd->SetWindowText(MAINDLG_UPPER_HASH);
	pWnd = GetDlgItem(IDC_STATIC_TIMETITLE);
	pWnd->SetWindowText(MAINDLG_TIME_TITLE);

	m_btnOpen.SetWindowText(MAINDLG_OPEN);
	m_btnStop.SetWindowText(MAINDLG_STOP);
	m_btnCopy.SetWindowText(MAINDLG_COPY);
	m_btnFind.SetWindowText(MAINDLG_VERIFY);
	m_btnExit.SetWindowText(MAINDLG_EXIT);
	pWnd = GetDlgItem(IDC_ABOUT);
	pWnd->SetWindowText(MAINDLG_ABOUT);

	m_uiBridgeMFC = new UIBridgeMFC(m_hWnd, &m_tstrAll, &m_mainMtx);

	m_mainMtx.lock();
	{
		m_tstrAll = _T("");

		m_thrdData.uiBridge = m_uiBridgeMFC;
		m_thrdData.uppercase = false;

		m_thrdData.nFiles = 0;

		m_thrdData.resultList.clear();
	}
	m_mainMtx.unlock();
	
	pTl = NULL;

	m_editMain.SetLimitText(UINT_MAX);
	m_editMain.SetWindowText(MAINDLG_INITINFO);

	if(ContextMenuExisted())
	{
		// 已经添加右键菜单
		m_btnContext.SetWindowText(MAINDLG_REMOVE_CONTEXT_MENU);
	}
	else
	{
		m_btnContext.SetWindowText(MAINDLG_ADD_CONTEXT_MENU);
	}
	pWnd = (CStatic*)GetDlgItem(IDC_STATIC_ADDRESULT);
	pWnd->SetWindowText(_T(""));

	SetCtrls(FALSE);

	// 从命令行获取文件路径
	TStrVector Paras = ParseCmdLine();
	ClearFilePaths();
	for(TStrVector::iterator ite = Paras.begin(); ite != Paras.end(); ++ite)
	{
		m_thrdData.fullPaths.push_back(*ite);
		++m_thrdData.nFiles;
	}
	// 从命令行获取文件路径结束
	
	m_thrdData.threadWorking = false;
	m_prog.SetRange(0, 99);
	m_progWhole.SetRange(0, 99);
	m_chkUppercase.SetCheck(0);

	if(m_thrdData.nFiles > 0)
		SetTimer(4, 50, NULL); // 使 DoMD5() 在 OnInitDialog() 之后执行

	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CFilesHashDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
HCURSOR CFilesHashDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CFilesHashDlg::OnDropFiles(HDROP hDropInfo)
{
	if(!m_thrdData.threadWorking)
	{
		unsigned int i;
		TCHAR tszDragFilename[MAX_PATH];
		DragAcceptFiles(FALSE);

		m_thrdData.nFiles = DragQueryFile(hDropInfo, -1, NULL, 0);
		ClearFilePaths();

		for(i = 0; i < m_thrdData.nFiles; i++)
		{
			DragQueryFile(hDropInfo, i, tszDragFilename, sizeof(tszDragFilename));
			tstring tmp = tszDragFilename;
			m_thrdData.fullPaths.push_back(tmp);
		}

		DragFinish(hDropInfo);
		DragAcceptFiles(TRUE);

		DoMD5();
	}
}

TStrVector CFilesHashDlg::ParseCmdLine()
{
	// 从命令行获取文件路径
	TStrVector parameters;

#if defined(UNICODE) || defined(_UNICODE)
	size_t cmdLen = wcslen(theApp.m_lpCmdLine);
#else
	size_t cmdLen = strlen(theApp.m_lpCmdLine);
#endif

	if(cmdLen > 0)
	{
		for(size_t i = 0; i < cmdLen; ++i)
		{
			tstring tstrPara(_T(""));
			if(theApp.m_lpCmdLine[i] == '"')
			{
				++i;
				for(; theApp.m_lpCmdLine[i] != '"'; ++i)
					tstrPara += theApp.m_lpCmdLine[i];
				parameters.push_back(tstrPara);
				++i;
			}
			else
			{
				for(; theApp.m_lpCmdLine[i] != ' '; ++i)
					tstrPara += theApp.m_lpCmdLine[i];
				parameters.push_back(tstrPara);
			}
		}
	}
	// 从命令行获取文件路径结束
	return parameters;
}

void CFilesHashDlg::PrepareAdvTaskbar()
{
	m_bAdvTaskbar = FALSE;
#ifndef _DEBUG
	VERIFY(CoCreateInstance(
			CLSID_TaskbarList, NULL, CLSCTX_ALL,
			IID_ITaskbarList3, (void**)&pTl));
	if(pTl)
		m_bAdvTaskbar = TRUE;
	else
		m_bAdvTaskbar = FALSE;
#endif
}

void CFilesHashDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if(m_thrdData.threadWorking)
	{
		m_waitingExit = TRUE;
		StopWorkingThread();

		return;
	}

	if(m_uiBridgeMFC != NULL)
	{
		delete m_uiBridgeMFC;
		m_uiBridgeMFC = NULL;
	}

	CDialog::OnClose();
}

void CFilesHashDlg::OnBnClickedOpen()
{
	if(!m_thrdData.threadWorking)
	{
		CString filter;
		TCHAR* nameBuffer;
		POSITION pos;
		nameBuffer = new TCHAR[MAX_FILES_NUM * MAX_PATH + 1];
		nameBuffer[0] = 0;
		filter = FILE_STRING;
		filter.Append(_T("(*.*)|*.*|"));
		CFileDialog dlgOpen(TRUE, NULL, NULL, OFN_HIDEREADONLY|OFN_ALLOWMULTISELECT, filter, NULL, 0);
		dlgOpen.GetOFN().lpstrFile = nameBuffer;
		dlgOpen.GetOFN().nMaxFile = MAX_FILES_NUM;
		if(IDOK == dlgOpen.DoModal())
		{
			pos = dlgOpen.GetStartPosition();
			ClearFilePaths();
			for(m_thrdData.nFiles = 0; pos != NULL; m_thrdData.nFiles++)
				m_thrdData.fullPaths.push_back(dlgOpen.GetNextPathName(pos).GetString());

			DoMD5();
		}
		delete[] nameBuffer;
	}
}

void CFilesHashDlg::OnBnClickedStop()
{
	if(m_thrdData.threadWorking)
	{
		//停止工作线程
		StopWorkingThread();
	}
}

void CFilesHashDlg::OnBnClickedExit()
{
	PostMessage(WM_CLOSE);//OnCancel();
}

void CFilesHashDlg::OnBnClickedAbout()
{
	CAboutDlg About;
	About.DoModal();
}

void CFilesHashDlg::OnBnClickedClean()
{
	if(!m_thrdData.threadWorking)
	{
		CString strBtnText;
		m_btnClr.GetWindowText(strBtnText);
		if(strBtnText.Compare(MAINDLG_CLEAR) == 0)
		{
			m_mainMtx.lock();
			{
				m_tstrAll = _T("");

				m_thrdData.resultList.clear();
			
				m_editMain.SetWindowText(m_tstrAll.c_str());
			}
			m_mainMtx.unlock();

			CStatic* pWnd =(CStatic *)GetDlgItem(IDC_STATIC_TIME);
			pWnd->SetWindowText(_T(""));
			pWnd = (CStatic*)GetDlgItem(IDC_STATIC_SPEED);
			pWnd->SetWindowText(_T(""));

			m_prog.SetPos(0);
			//m_progWhole.SetPos(0);
			SetWholeProgPos(0);
		}
		else if(strBtnText.Compare(MAINDLG_CLEAR_VERIFY) == 0)
		{
			m_bFind = FALSE; // 退出搜索模式
			m_btnClr.SetWindowText(MAINDLG_CLEAR);

			RefreshResult();
			RefreshMainText();
		}
	}
}

void CFilesHashDlg::OnBnClickedCopy()
{
	if(!m_thrdData.threadWorking)
	{
		m_editMain.SetSel(0, -1);
		m_editMain.Copy();
		m_editMain.SetSel(-1,0);
		m_editMain.LineScroll(m_editMain.GetLineCount()); // 将文本框滚动到结尾
	}
}

void CFilesHashDlg::OnBnClickedFind()
{
	// TODO: 在此添加控件通知处理程序代码
	CFindDlg Find;
	Find.SetFindHash(_T(""));
	if(IDOK == Find.DoModal())
	{
		m_strFindHash = Find.GetFindHash().Trim();
		if(m_strFindHash.Compare(_T("")) != 0)
		{
			m_bFind = TRUE; // 进入搜索模式
			m_btnClr.SetWindowText(MAINDLG_CLEAR_VERIFY);
			//m_editMain.SetWindowText(_T(""));
			tstring tstrResult = ResultFind(m_strFindFile, m_strFindHash);
			m_editMain.SetWindowText(tstrResult.c_str());
			//m_editMain.LineScroll(m_editMain.GetLineCount()); // 将文本框滚动到结尾
		}
	}
}

void CFilesHashDlg::OnBnClickedContext()
{
	if(m_bLimited)
	{
		OSVERSIONINFOEX osvi;
		BOOL bOsVersionInfoEx;
		if(GetWindowsVersion(osvi, bOsVersionInfoEx) &&
			osvi.dwMajorVersion >= 6)
		{
			if(ElevateProcess())
				ExitProcess(0);
		}
	}

	// May not a limited process.
	CStatic* pWnd = (CStatic *)GetDlgItem(IDC_STATIC_ADDRESULT);
	CString buttonText = _T("");

	m_btnContext.GetWindowText(buttonText);

	if(buttonText.Compare(MAINDLG_ADD_CONTEXT_MENU) == 0)
	{
		RemoveContextMenu(); // Try to delete all items related to fHash
		if(AddContextMenu())
		{
			pWnd->SetWindowText(MAINDLG_ADD_SUCCEEDED);
			m_btnContext.SetWindowText(MAINDLG_REMOVE_CONTEXT_MENU);
		}
		else
		{
			pWnd->SetWindowText(MAINDLG_ADD_FAILED);
		}
	}
	else if(buttonText.Compare(MAINDLG_REMOVE_CONTEXT_MENU) == 0)
	{
		if(RemoveContextMenu())
		{
			pWnd->SetWindowText(MAINDLG_REMOVE_SUCCEEDED);
			m_btnContext.SetWindowText(MAINDLG_ADD_CONTEXT_MENU);
		}
		else
		{
			pWnd->SetWindowText(MAINDLG_REMOVE_FAILED);
		}
	}
}

void CFilesHashDlg::OnBnClickedCheckup()
{
	// Remember current scroll position
	int iFirstVisible = m_editMain.GetFirstVisibleLine();

	if(!m_bFind)
	{
		// List mode
		RefreshResult();
		RefreshMainText(FALSE);
	}
	else
	{
		// Search mode
		tstring tstrResult = ResultFind(m_strFindFile, m_strFindHash);
		m_editMain.SetWindowText(tstrResult.c_str());
	}

	// Reset scroll position
	m_editMain.LineScroll(iFirstVisible);
}

void CFilesHashDlg::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 1)
	{
		// 计算花费时间
		m_calculateTime += 0.1f;
		CStatic* pWnd = (CStatic*)GetDlgItem(IDC_STATIC_TIME);
		CString cstrTime;
		int i_calculateTime = (int)m_calculateTime;
		CString cstrFormat("%d ");
		cstrFormat.Append(SECOND_STRING);
		cstrTime.Format(cstrFormat, i_calculateTime);
		pWnd->SetWindowText(cstrTime);
	}
	else if(nIDEvent == 4)
	{
		// 通过命令行启动的
		DoMD5();
		KillTimer(4);
	}

	CDialog::OnTimer(nIDEvent);
}

void CFilesHashDlg::SetWholeProgPos(UINT pos)
{
	m_progWhole.SetPos(pos);
	if(m_bAdvTaskbar)
		pTl->SetProgressValue(this->m_hWnd, pos, 99);
}

void CFilesHashDlg::DoMD5()
{
	if(m_hWorkThread)
	{
		CloseHandle(m_hWorkThread);
	}
	
	m_bFind = FALSE;
	m_btnClr.SetWindowText(MAINDLG_CLEAR);

	PrepareAdvTaskbar();

	m_prog.SetPos(0);
	//m_progWhole.SetPos(0);
	SetWholeProgPos(0);

	m_thrdData.uppercase = (m_chkUppercase.GetCheck() != FALSE);

	m_calculateTime = 0.0;
	m_timer = SetTimer(1, 100, NULL);
	CStatic* pWnd = (CStatic*)GetDlgItem(IDC_STATIC_TIME);
	CString cstrZero(_T("0 "));
	cstrZero.Append(SECOND_STRING);
	pWnd->SetWindowText(cstrZero);
	pWnd = (CStatic*)GetDlgItem(IDC_STATIC_SPEED);
	pWnd->SetWindowText(_T(""));

	DWORD thredID;

	m_thrdData.stop = false;
	m_hWorkThread = (HANDLE)_beginthreadex(NULL, 
											0, 
											(unsigned int (WINAPI *)(void *))HashThreadFunc, 
											&m_thrdData, 
											0, 
											(unsigned int *)&thredID);

}

void CFilesHashDlg::StopWorkingThread()
{
	if(m_thrdData.threadWorking)
	{
		m_thrdData.stop = true;
	}
}

HBRUSH CFilesHashDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此更改 DC 的任何属性

	// TODO:  如果默认的不是所需画笔，则返回另一个画笔
	if(pWnd->GetDlgCtrlID() == IDE_TXTMAIN)
	{
		pDC->SetBkMode(TRANSPARENT);


		hbr = m_bruEditBkg;
	}
	return hbr;
}

void CFilesHashDlg::ClearFilePaths()
{
	m_thrdData.fullPaths.clear();
}

void CFilesHashDlg::SetCtrls(BOOL working)
{
	if(working)
	{
		DragAcceptFiles(FALSE);
		m_btnOpen.EnableWindow(FALSE);
		m_btnStop.EnableWindow(TRUE);
		m_btnClr.EnableWindow(FALSE);
		m_btnCopy.EnableWindow(FALSE);
		m_btnFind.EnableWindow(FALSE);
		m_btnContext.EnableWindow(FALSE);
		m_chkUppercase.EnableWindow(FALSE);
		GotoDlgCtrl(&m_btnStop);
	}
	else
	{
		m_btnOpen.EnableWindow(TRUE);
		m_btnStop.EnableWindow(FALSE);
		m_btnClr.EnableWindow(TRUE);
		m_btnCopy.EnableWindow(TRUE);
		m_btnFind.EnableWindow(TRUE);
		if(m_bLimited)
		{
			Button_SetElevationRequiredState(m_btnContext.GetSafeHwnd(), TRUE);
			//m_btnContext.SetWindowText(MAINDLG_CONTEXT_INIT);
			//m_btnContext.EnableWindow(FALSE);
		}
		else
		{
			Button_SetElevationRequiredState(m_btnContext.GetSafeHwnd(), FALSE);
			//m_btnContext.EnableWindow(TRUE);
		}
		m_btnContext.EnableWindow(TRUE);
		m_chkUppercase.EnableWindow(TRUE);
		GotoDlgCtrl(&m_btnOpen);
		DragAcceptFiles();
	}
}

void CFilesHashDlg::RefreshResult()
{
	m_tstrAll = _T("");
	ResultList::iterator itr = m_thrdData.resultList.begin();
	for(; itr != m_thrdData.resultList.end(); ++itr)
	{
		AppendResult(*itr, m_tstrAll);
	}
}

void CFilesHashDlg::RefreshMainText(BOOL bScrollToEnd /*= TRUE*/)
{
	m_mainMtx.lock();
	m_editMain.SetWindowText(m_tstrAll.c_str());
	m_mainMtx.unlock();
	if(bScrollToEnd)
		m_editMain.LineScroll(m_editMain.GetLineCount()); // 将文本框滚动到结尾
}

void CFilesHashDlg::CalcSpeed(ULONGLONG tsize)
{
	KillTimer(m_timer);
	double speed;
	if(m_calculateTime > 0.1)
	{
		speed = tsize / m_calculateTime;
		CString speedStr, measure = _T("B/s");
		if((speed / 1024) > 1)
		{
			speed /= 1024;
			measure = _T("KB/s");
			if((speed / 1024) > 1)
			{
				speed /= 1024;
				measure = _T("MB/s");
			}
		}
		speedStr.Format(_T("%4.2f "), speed);
		CStatic* pWnd = (CStatic*)GetDlgItem(IDC_STATIC_SPEED);
		speedStr.Append(measure);
		pWnd->SetWindowText(speedStr);
	}
	else
	{
		CStatic* pWnd = (CStatic*)GetDlgItem(IDC_STATIC_SPEED);
		pWnd->SetWindowText(_T(""));
	}
}

LRESULT CFilesHashDlg::OnThreadMsg(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{
	case WP_WORKING:
		SetCtrls(TRUE);
		break;
	case WP_REFRESH_TEXT:
		RefreshMainText();
		break;
	case WP_PROG:
		m_prog.SetPos((int)lParam);
		break;
	case WP_PROG_WHOLE:
		SetWholeProgPos((int)lParam);
		break;
	case WP_FINISHED:
		// 停止主界面计时器 计算读取速度
		CalcSpeed(m_thrdData.totalSize);
		// 停止主界面计时器 计算读取速度

		// 界面设置 - 开始
		SetCtrls(FALSE);
		// 界面设置 - 结束	

		m_prog.SetPos(99);
		SetWholeProgPos(99);
		break;
	case WP_STOPPED:
		KillTimer(1);

		m_calculateTime = 0.0;
		CStatic* pWnd = (CStatic*)GetDlgItem(IDC_STATIC_TIME);
		pWnd->SetWindowText(_T(""));

		//界面设置 - 开始
		SetCtrls(FALSE);
		//界面设置 - 结束
		
		m_mainMtx.lock();
		{
			m_tstrAll.append(_T("\r\n"));
			m_tstrAll.append(MAINDLG_CALCU_TERMINAL);
			m_tstrAll.append(_T("\r\n\r\n"));

			m_editMain.SetWindowText(m_tstrAll.c_str());
		}
		m_mainMtx.unlock();
			
		m_editMain.LineScroll(m_editMain.GetLineCount()); // 将文本框滚动到结尾

		m_prog.SetPos(0);
		//m_progWhole.SetPos(0);
		SetWholeProgPos(0);
		
		if(m_waitingExit)
		{
			PostMessage(WM_CLOSE);
		}
		break;
	}

	return 0;
}

tstring CFilesHashDlg::ResultFind(CString strFile, CString strHash)
{
	tstring tstrResult(MAINDLG_FIND_IN_RESULT);
	tstrResult.append(_T("\r\n"));
	tstrResult.append(HASHVALUE_STRING);
	tstrResult.append(_T(" "));
	tstrResult.append(strHash);
	tstrResult.append(_T("\r\n\r\n"));
	tstrResult.append(MAINDLG_RESULT);
	tstrResult.append(_T("\r\n\r\n"));

	strHash.MakeUpper();
	strFile.MakeLower();

	size_t count = 0;
	CString strPathLower;
	ResultList::iterator itr = m_thrdData.resultList.begin();
	for(; itr != m_thrdData.resultList.end(); ++itr)
	{
		strPathLower = CString(itr->tstrPath.c_str());
		strPathLower.MakeLower();
		if(strPathLower.Find(strFile) >= 0 && 
			(itr->tstrMD5.find(strHash.GetString()) != tstring::npos ||
			itr->tstrSHA1.find(strHash.GetString()) != tstring::npos ||
			itr->tstrSHA256.find(strHash.GetString()) != tstring::npos ||
			itr->tstrCRC32.find(strHash.GetString()) != tstring::npos ))
		{
			++count;

			AppendResult(*itr, tstrResult);
		}
	}

	if(count == 0)
		tstrResult.append(MAINDLG_NORESULT);

	return tstrResult;
}

void CFilesHashDlg::AppendResult(const ResultData& result, tstring& tstrToAppend)
{
	tstrToAppend.append(FILENAME_STRING);
	tstrToAppend.append(_T(" "));
	tstrToAppend.append(result.tstrPath);
	tstrToAppend.append(_T("\r\n"));
	if(result.bDone)
	{
		// A succeed result
		tstrToAppend.append(FILESIZE_STRING);
		tstrToAppend.append(_T(" "));
		
		char chSize[1024] = {0};
		sprintf_s(chSize, 1024, "%I64u ", result.ulSize);

		tstrToAppend.append(strtotstr(string(chSize)));
		tstrToAppend.append(BYTE_STRING);

		tstring tstrResultSize = strtotstr(ConvertSizeToStr(result.ulSize));
		tstrToAppend.append(tstrResultSize);
		tstrToAppend.append(_T("\r\n"));
		tstrToAppend.append(MODIFYTIME_STRING);
		tstrToAppend.append(_T(" "));
		tstrToAppend.append(result.tstrMDate.c_str());
		if(result.tstrVersion != tstring(_T("")))
		{
			tstrToAppend.append(_T("\r\n"));
			tstrToAppend.append(VERSION_STRING);
			tstrToAppend.append(_T(" "));
			tstrToAppend.append(result.tstrVersion.c_str());
		}
		tstrToAppend.append(_T("\r\n"));

		CString strMD5(result.tstrMD5.c_str());
		CString strSHA1(result.tstrSHA1.c_str());
		CString strSHA256(result.tstrSHA256.c_str());
		CString strCRC32(result.tstrCRC32.c_str());

		if(m_thrdData.uppercase = (m_chkUppercase.GetCheck() == TRUE))
		{
			tstrToAppend.append(_T("MD5: "));
			tstrToAppend.append(strMD5.MakeUpper());
			tstrToAppend.append(_T("\r\nSHA1: "));
			tstrToAppend.append(strSHA1.MakeUpper());
			tstrToAppend.append(_T("\r\nSHA256: "));
			tstrToAppend.append(strSHA256.MakeUpper());
			tstrToAppend.append(_T("\r\nCRC32: "));
			tstrToAppend.append(strCRC32.MakeUpper());
		}
		else
		{
			tstrToAppend.append(_T("MD5: "));
			tstrToAppend.append(strMD5.MakeLower());
			tstrToAppend.append(_T("\r\nSHA1: "));
			tstrToAppend.append(strSHA1.MakeLower());
			tstrToAppend.append(_T("\r\nSHA256: "));
			tstrToAppend.append(strSHA256.MakeLower());
			tstrToAppend.append(_T("\r\nCRC32: "));
			tstrToAppend.append(strCRC32.MakeLower());
		}
	}
	else
	{
		// An error result
		tstrToAppend.append(result.tstrError.c_str());
	}

	tstrToAppend.append(_T("\r\n\r\n"));
}
