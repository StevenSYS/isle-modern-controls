#include <iostream>

#include "legoinputmanager.h"

#include "legocameracontroller.h"
#include "legocontrolmanager.h"
#include "legomain.h"
#include "legoutils.h"
#include "legovideomanager.h"
#include "legoworld.h"
#include "misc.h"
#include "mxautolock.h"
#include "mxdebug.h"
#include "roi/legoroi.h"

DECOMP_SIZE_ASSERT(LegoInputManager, 0x338)
DECOMP_SIZE_ASSERT(LegoNotifyList, 0x18)
DECOMP_SIZE_ASSERT(LegoNotifyListCursor, 0x10)
DECOMP_SIZE_ASSERT(LegoEventQueue, 0x18)

// GLOBAL: LEGO1 0x100f31b0
MxS32 g_unk0x100f31b0 = -1;

// GLOBAL: LEGO1 0x100f31b4
const char* g_unk0x100f31b4 = NULL;

// GLOBAL: LEGO1 0x100f67b8
MxBool g_unk0x100f67b8 = TRUE;

// FUNCTION: LEGO1 0x1005b790
LegoInputManager::LegoInputManager()
{
	m_keyboardNotifyList = NULL;
	m_world = NULL;
	m_camera = NULL;
	m_eventQueue = NULL;
	m_unk0x80 = FALSE;
	m_autoDragTimerID = 0;
	m_x = 0;
	m_y = 0;
	m_controlManager = NULL;
	m_unk0x81 = FALSE;
	m_unk0x88 = FALSE;
	m_directInput = NULL;
	m_directInputDevice = NULL;
	m_kbStateSuccess = FALSE;
	m_turnLock = 0;
	m_joyid = -1;
	m_unk0x335 = FALSE;
	m_unk0x336 = FALSE;
	m_unk0x74 = 0x19;
	m_autoDragTime = 1000;
}

// FUNCTION: LEGO1 0x1005b8f0
LegoInputManager::~LegoInputManager()
{
	Destroy();
}

// FUNCTION: LEGO1 0x1005b960
MxResult LegoInputManager::Create(HWND p_hwnd)
{
	MxResult result = SUCCESS;

	m_controlManager = new LegoControlManager;

	if (!m_keyboardNotifyList) {
		m_keyboardNotifyList = new LegoNotifyList;
	}

	if (!m_eventQueue) {
		m_eventQueue = new LegoEventQueue;
	}

	CreateAndAcquireKeyboard(p_hwnd);

	if (!m_keyboardNotifyList || !m_eventQueue || !m_directInputDevice) {
		Destroy();
		result = FAILURE;
	}

	return result;
}

// FUNCTION: LEGO1 0x1005bfe0
void LegoInputManager::Destroy()
{
	ReleaseDX();

	if (m_keyboardNotifyList) {
		delete m_keyboardNotifyList;
	}
	m_keyboardNotifyList = NULL;

	if (m_eventQueue) {
		delete m_eventQueue;
	}
	m_eventQueue = NULL;

	if (m_controlManager) {
		delete m_controlManager;
	}
}

// FUNCTION: LEGO1 0x1005c030
// FUNCTION: BETA10 0x10088f6e
void LegoInputManager::CreateAndAcquireKeyboard(HWND p_hwnd)
{
	HINSTANCE hinstance = (HINSTANCE) GetWindowLong(p_hwnd, GWL_HINSTANCE);

	// 0x500 for DX5
	if (DirectInputCreate(hinstance, 0x500, &m_directInput, NULL) == DI_OK) {
		if (m_directInput->CreateDevice(GUID_SysKeyboard, &m_directInputDevice, NULL) == DI_OK) {
			m_directInputDevice->SetCooperativeLevel(p_hwnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
			m_directInputDevice->SetDataFormat(&c_dfDIKeyboard);
			if (m_directInputDevice->Acquire()) {
				MxTrace("Can't acquire the keyboard!\n");
			}
		}
	}
}

// FUNCTION: LEGO1 0x1005c0a0
void LegoInputManager::ReleaseDX()
{
	if (m_directInputDevice != NULL) {
		m_directInputDevice->Unacquire();
		m_directInputDevice->Release();
		m_directInputDevice = NULL;
	}

	if (m_directInput != NULL) {
		m_directInput->Release();
		m_directInput = NULL;
	}
}

// FUNCTION: LEGO1 0x1005c0f0
void LegoInputManager::GetKeyboardState()
{
	m_kbStateSuccess = FALSE;

	if (m_directInputDevice) {
		HRESULT hr = m_directInputDevice->GetDeviceState(sizeOfArray(m_keyboardState), &m_keyboardState);

		if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
			if (m_directInputDevice->Acquire() == S_OK) {
				hr = m_directInputDevice->GetDeviceState(sizeOfArray(m_keyboardState), &m_keyboardState);
			}
		}

		if (hr == S_OK) {
			m_kbStateSuccess = TRUE;
		}
	}
}

// FUNCTION: LEGO1 0x1005c160
MxResult LegoInputManager::GetNavigationKeyStates(MxU32& p_keyFlags)
{
	GetKeyboardState();

	if (!m_kbStateSuccess) {
		return FAILURE;
	}

	if (g_unk0x100f67b8) {
		if (m_keyboardState[DIK_A] & 0x80 && GetAsyncKeyState('A') == 0) {
			m_keyboardState[DIK_A] = 0;
		}

		if (m_keyboardState[DIK_D] & 0x80 && GetAsyncKeyState('D') == 0) {
			m_keyboardState[DIK_D] = 0;
		}
	}

	MxU32 keyFlags = 0;

	if (m_keyboardState[DIK_W] & 0x80) {
		keyFlags |= c_up;
	}

	if (m_keyboardState[DIK_S] & 0x80) {
		keyFlags |= c_down;
	}

	if (m_keyboardState[DIK_A] & 0x80) {
		keyFlags |= c_left;
	}

	if (m_keyboardState[DIK_D] & 0x80) {
		keyFlags |= c_right;
	}

	if ((m_keyboardState[DIK_LCONTROL] | m_keyboardState[DIK_RCONTROL]) & 0x80) {
		keyFlags |= c_bit5;
	}

	p_keyFlags = keyFlags;

	return SUCCESS;
}

// FUNCTION: LEGO1 0x1005c470
void LegoInputManager::Register(MxCore* p_notify)
{
	AUTOLOCK(m_criticalSection);

	LegoNotifyListCursor cursor(m_keyboardNotifyList);
	if (!cursor.Find(p_notify)) {
		m_keyboardNotifyList->Append(p_notify);
	}
}

// FUNCTION: LEGO1 0x1005c5c0
void LegoInputManager::UnRegister(MxCore* p_notify)
{
	AUTOLOCK(m_criticalSection);

	LegoNotifyListCursor cursor(m_keyboardNotifyList);
	if (cursor.Find(p_notify)) {
		cursor.Detach();
	}
}

// FUNCTION: LEGO1 0x1005c700
void LegoInputManager::SetCamera(LegoCameraController* p_camera)
{
	m_camera = p_camera;
}

// FUNCTION: LEGO1 0x1005c710
void LegoInputManager::ClearCamera()
{
	m_camera = NULL;
}

// FUNCTION: LEGO1 0x1005c720
// FUNCTION: BETA10 0x100896b8
void LegoInputManager::SetWorld(LegoWorld* p_world)
{
	m_world = p_world;
}

// FUNCTION: LEGO1 0x1005c730
// FUNCTION: BETA10 0x100896dc
void LegoInputManager::ClearWorld()
{
	m_world = NULL;
}

// FUNCTION: LEGO1 0x1005c740
void LegoInputManager::QueueEvent(NotificationId p_id, MxU8 p_modifier, MxLong p_x, MxLong p_y, MxU8 p_key)
{
	LegoEventNotificationParam param = LegoEventNotificationParam(p_id, NULL, p_modifier, p_x, p_y, p_key);

	if (((!m_unk0x88) || ((m_unk0x335 && (param.GetNotification() == c_notificationButtonDown)))) ||
		((m_unk0x336 && (p_key == VK_SPACE)))) {
		ProcessOneEvent(param);
	}
}

// FUNCTION: LEGO1 0x1005c820
void LegoInputManager::ProcessEvents()
{
	AUTOLOCK(m_criticalSection);

	LegoEventNotificationParam event;
	while (m_eventQueue->Dequeue(event)) {
		if (ProcessOneEvent(event)) {
			break;
		}
	}
}

// FUNCTION: LEGO1 0x1005c9c0
MxBool LegoInputManager::ProcessOneEvent(LegoEventNotificationParam& p_param)
{
	MxBool processRoi;

	if (p_param.GetNotification() == c_notificationKeyPress) {
		if (!Lego()->IsPaused() || p_param.GetKey() == VK_PAUSE) {
			LegoNotifyListCursor cursor(m_keyboardNotifyList);
			MxCore* target;

			while (cursor.Next(target)) {
				if (target->Notify(p_param) != 0) {
					return TRUE;
				}
			}
		}
	}
	else {
		if (!Lego()->IsPaused()) {
			processRoi = TRUE;

			if (m_unk0x335 != 0) {
				if (p_param.GetNotification() == c_notificationButtonDown) {
					LegoEventNotificationParam notification(c_notificationKeyPress, NULL, 0, 0, 0, VK_SPACE);
					LegoNotifyListCursor cursor(m_keyboardNotifyList);
					MxCore* target;

					while (cursor.Next(target)) {
						if (target->Notify(notification) != 0) {
							return TRUE;
						}
					}
				}

				return TRUE;
			}	

			if (m_world != NULL && m_world->Notify(p_param) != 0) {
				return TRUE;
			}

			if (p_param.GetNotification() == c_notificationButtonDown) {
				MxPresenter* presenter = VideoManager()->GetPresenterAt(p_param.GetX(), p_param.GetY());

				if (presenter) {
					if (presenter->GetDisplayZ() < 0) {
						processRoi = FALSE;

						if (m_controlManager->FUN_10029210(p_param, presenter)) {
							return TRUE;
						}
					}
					else {
						LegoROI* roi = PickROI(p_param.GetX(), p_param.GetY());

						if (roi == NULL && m_controlManager->FUN_10029210(p_param, presenter)) {
							return TRUE;
						}
					}
				}
			}
			else if (p_param.GetNotification() == c_notificationButtonUp) {
				if (g_unk0x100f31b0 != -1 || m_controlManager->GetUnknown0x10() ||
					m_controlManager->GetUnknown0x0c() == 1) {
					MxBool result = m_controlManager->FUN_10029210(p_param, NULL);
					StopAutoDragTimer();

					m_unk0x80 = FALSE;
					m_unk0x81 = FALSE;
					return result;
				}
			}

			if (FUN_1005cdf0(p_param)) {
				if (processRoi && p_param.GetNotification() == c_notificationClick) {
					LegoROI* roi = PickROI(p_param.GetX(), p_param.GetY());
					p_param.SetROI(roi);

					if (roi && roi->GetVisibility() == TRUE) {
						for (OrientableROI* parent = roi->GetParentROI(); parent; parent = parent->GetParentROI()) {
							roi = (LegoROI*) parent;
						}

						LegoEntity* entity = roi->GetEntity();
						if (entity && entity->Notify(p_param) != 0) {
							return TRUE;
						}
					}
				}

				if (m_camera && m_camera->Notify(p_param) != 0) {
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

// FUNCTION: LEGO1 0x1005cdf0
// FUNCTION: BETA10 0x10089cc1
MxBool LegoInputManager::FUN_1005cdf0(LegoEventNotificationParam& p_param)
{
	MxBool result = FALSE;

	switch (p_param.GetNotification()) {
	case c_notificationButtonDown:
		m_x = p_param.GetX();
		m_y = p_param.GetY();
		m_unk0x80 = FALSE;
		m_unk0x81 = TRUE;
		StartAutoDragTimer();
		break;
	case c_notificationButtonUp:
		StopAutoDragTimer();

		if (m_unk0x80) {
			p_param.SetNotification(c_notificationDragEnd);
			result = TRUE;
		}
		else if (m_unk0x81) {
			p_param.SetX(m_x);
			p_param.SetY(m_y);
			p_param.SetNotification(c_notificationClick);
			result = TRUE;
		}

		m_unk0x80 = FALSE;
		m_unk0x81 = FALSE;
		break;
	case c_notificationMouseLocked:
		m_turnLock = TRUE;
		break;
	case c_notificationMouseUnlocked:
		m_unk0x80 = FALSE;
		p_param.SetNotification(c_notificationDragEnd);

		if (m_camera) {
			m_camera->Notify(p_param);
		}

		m_turnLock = FALSE;
		break;
	case c_notificationMouseMove:
		if (m_turnLock) {
			p_param.SetModifier(LegoEventNotificationParam::c_lButtonState);
		}

		if (m_turnLock && p_param.GetModifier() & LegoEventNotificationParam::c_lButtonState) {
			if (!m_unk0x80) {
				if (m_turnLock) {
					m_x = p_param.GetX();
					m_y = p_param.GetY();
				}

				MxS32 diffX = p_param.GetX() - m_x;
				MxS32 diffY = p_param.GetY() - m_y;
				MxS32 t = diffX * diffX + diffY * diffY;

				if (m_turnLock || t > m_unk0x74) {
					StopAutoDragTimer();
					m_unk0x80 = TRUE;
					p_param.SetNotification(c_notificationDragStart);
					result = TRUE;
					p_param.SetX(m_x);
					p_param.SetY(m_y);
				}
			}
			else {
				p_param.SetNotification(c_notificationDrag);
				result = TRUE;
			}
		}
		break;
	case c_notificationTimer:
		if (p_param.GetModifier() == m_autoDragTimerID) {
			StopAutoDragTimer();

			if (m_unk0x81) {
				m_unk0x80 = TRUE;
				p_param.SetX(m_x);
				p_param.SetY(m_y);
				p_param.SetModifier(LegoEventNotificationParam::c_lButtonState);
				p_param.SetNotification(c_notificationDragStart);
				result = TRUE;
			}
			else {
				m_unk0x80 = FALSE;
			}
		}
		break;
	}

	return result;
}

// FUNCTION: LEGO1 0x1005cfb0
// FUNCTION: BETA10 0x10089fc5
void LegoInputManager::StartAutoDragTimer()
{
	m_autoDragTimerID = ::SetTimer(LegoOmni::GetInstance()->GetWindowHandle(), 1, m_autoDragTime, NULL);
}

// FUNCTION: LEGO1 0x1005cfd0
// FUNCTION: BETA10 0x1008a005
void LegoInputManager::StopAutoDragTimer()
{
	if (m_autoDragTimerID) {
		::KillTimer(LegoOmni::GetInstance()->GetWindowHandle(), m_autoDragTimerID);
	}
}

// FUNCTION: LEGO1 0x1005cff0
void LegoInputManager::EnableInputProcessing()
{
	m_unk0x88 = FALSE;
	g_unk0x100f31b0 = -1;
	g_unk0x100f31b4 = NULL;
}