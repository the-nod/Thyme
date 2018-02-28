/**
 * @file
 *
 * @author OmniBlade
 *
 * @brief Base class for mouse input handlers.
 *
 * @copyright Thyme is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "mouse.h"
#include "displaystringmanager.h"
#include "globaldata.h"
#include "ini.h"
#include "keyboard.h"
#include "messagestream.h"
#include "minmax.h"
#include "rtsutils.h"

using rts::Get_Time;

#ifndef THYME_STANDALONE
extern Mouse *&g_theMouse = Make_Global<Mouse *>(0x00A29B60);
#else
extern Mouse *g_theMouse = nullptr;
#endif

/**
 * 0x004029D0
 */
Mouse::Mouse() :
    m_tooltipFontName("Times New Roman"),
    m_tooltipFontSize(12),
    m_tooltipFontIsBold(false),
    // m_tooltipFont("Times New Roman"),
    m_tooltipAnimateBackground(true),
    m_tooltipFillTime(50),
    m_tooltipDelayTime(50),
    m_toolTipwidth(15.0f),
    unkFloat(0.0f),
    m_tooltipColorText{220, 220, 220, 255},
    m_tooltipColorHighlight{255, 255, 0, 255},
    m_tooltipColorShadow{0, 0, 0, 255},
    m_tooltipColorBackground{20, 20, 0, 127},
    m_tooltipColorBorder{0, 0, 0, 255},
    m_currentRedrawMode(g_theWriteableGlobalData == nullptr || g_theWriteableGlobalData->m_winCursors ? RM_WINDOWS : RM_W3D),
    m_useTooltipAltTextColor(false),
    m_useTooltipAltBackColor(false),
    m_adjustTooltipAltColor(false),
    m_orthoCamera(false),
    m_orthoZoom(1.0f),
    m_dragTolerance(0),
    m_dragTolerance3D(0),
    m_dragToleranceMS(0),
    m_numButtons(0),
    m_numAxes(0),
    m_forceFeedback(false),
    m_tooltipString(),
    m_tooltipDisplayString(nullptr),
    m_displayTooltip(false),
    m_isTooltipEmpty(true),
    m_minX(0),
    m_maxX(0),
    m_minY(0),
    m_maxY(0),
    m_inputFrame(0),
    m_deadInputFrame(0),
    m_inputMovesAbsolute(false),
    m_visible(false),
    m_currentCursor(UNKNOWN),
    m_cursorTextDisplayString(nullptr),
    m_cursorTextColor{255, 255, 255, 255},
    m_cursorTextDropColor{255, 255, 255, 255},
    m_tooltipDelay(-1),
    unkInt1(0),
    unkInt2(0),
    m_stillTime(0),
    m_tooltipColorTextCopy{255, 255, 255, 255},
    m_tooltipColorBackgroundCopy{0, 0, 0, 255},
    unkInt3(0)
{
    memset(m_mouseEvents, 0, sizeof(m_mouseEvents));
    memset(&m_currMouse, 0, sizeof(m_currMouse));
    memset(&m_prevMouse, 0, sizeof(m_prevMouse));
}

/**
 * 0x00402E50
 */
Mouse::~Mouse()
{
    if (m_tooltipDisplayString != nullptr) {
        g_theDisplayStringManger->Free_Display_String(m_tooltipDisplayString);
    }

    m_tooltipDisplayString = nullptr;

    if (m_cursorTextDisplayString != nullptr) {
        g_theDisplayStringManger->Free_Display_String(m_cursorTextDisplayString);
    }

    m_cursorTextDisplayString = nullptr;
}

/**
 * @brief Initialises the subsystem.
 *
 * 0x00402FC0
 */
void Mouse::Init()
{
    m_currentRedrawMode =
        g_theWriteableGlobalData == nullptr || g_theWriteableGlobalData->m_winCursors ? RM_WINDOWS : m_currentRedrawMode;
    m_numButtons = 2;
    m_numAxes = 2;
    m_forceFeedback = false;
    Notify_Resolution_Change();
    m_tooltipString.Clear();
    m_displayTooltip = false;
    memset(m_mouseEvents, 0, sizeof(m_mouseEvents));
    memset(&m_currMouse, 0, sizeof(m_currMouse));
    memset(&m_prevMouse, 0, sizeof(m_prevMouse));
    m_minX = 0;
    m_maxX = 799; // Width of default 800x600 res - 1;
    m_minY = 0;
    m_maxY = 599; // Height of default 800x600 res - 1;
    m_inputFrame = 0;
    m_deadInputFrame = 0;
    m_inputMovesAbsolute = 0;
    unkInt3 = 0;
    m_currentCursor = UNKNOWN;
    m_cursorTextDisplayString = g_theDisplayStringManger->New_Display_String();
}

/**
 * @brief Resets the subsystem.
 *
 * 0x00403160
 */
void Mouse::Reset()
{
    if (m_cursorTextDisplayString != nullptr) {
        m_cursorTextDisplayString->Reset();
    }
}

/**
 * @brief Updates the subsystem.
 *
 * 0x00403170
 */
void Mouse::Update()
{
    Update_Mouse_Data();
}

/**
 * @brief Parses mouse configuration data from Data/INI/Mouse.ini.
 *
 * 0x00402F40
 */
void Mouse::Parse_INI()
{
    INI ini;
    ini.Load("Data/INI/Mouse.ini", INI_LOAD_OVERWRITE, nullptr);
}

/**
 * @brief Adds input messages to the message stream based on recorded mouse events.
 *
 * 0x004031F0
 */
void Mouse::Create_Stream_Messages()
{
    if (g_theMessageStream != nullptr) {
        unsigned call_time = Get_Time();
        GameMessage *start_msg = g_theMessageStream->Append_Message(GameMessage::MSG_RAW_MOUSE_BEGIN);
        start_msg->Append_Pixel_Arg(m_currMouse.pos);
        start_msg->Append_Int_Arg(g_theKeyboard->Get_Modifiers());
        unsigned delay = 0;

        if (!g_theWriteableGlobalData->m_scriptDebug) {
            delay = m_tooltipDelay < 0 ? m_tooltipDelayTime : m_tooltipDelay;
        }

        // See if the amont of time the mouse has been still is enough to setup displaying tool tip.
        if (call_time - m_stillTime < delay) {
            m_displayTooltip = false;
        } else {
            if (!m_displayTooltip) {
                unkInt1 = 0;
                unkInt2 = Get_Time();
            }

            m_displayTooltip = true;
        }

        for (int i = 0; i < unkInt3; ++i) {
            Process_Mouse_Event(i);

            // If the mouse has moved, set m_stillTime to be the time we got at function start to time elapsed time since
            // last move.
            if (m_currMouse.delta_pos.x != 0 || m_currMouse.delta_pos.y != 0) {
                m_stillTime = call_time;
            }

            GameMessage *event_msg;

            switch (m_currMouse.left_event) {
                case 5:
                    event_msg = g_theMessageStream->Append_Message(GameMessage::MSG_RAW_MOUSE_LEFT_BUTTON_DOWN);
                    event_msg->Append_Pixel_Arg(m_currMouse.pos);
                    event_msg->Append_Int_Arg(g_theKeyboard->Get_Modifiers());
                    event_msg->Append_Int_Arg(m_currMouse.wheel_pos);
                case 6:
                    event_msg = g_theMessageStream->Append_Message(GameMessage::MSG_RAW_MOUSE_LEFT_BUTTON_UP);
                    event_msg->Append_Pixel_Arg(m_currMouse.pos);
                    event_msg->Append_Int_Arg(g_theKeyboard->Get_Modifiers());
                    event_msg->Append_Int_Arg(m_currMouse.wheel_pos);
                case 7:
                    event_msg = g_theMessageStream->Append_Message(GameMessage::MSG_RAW_MOUSE_LEFT_DOUBLE_CLICK);
                    event_msg->Append_Pixel_Arg(m_currMouse.pos);
                    event_msg->Append_Int_Arg(g_theKeyboard->Get_Modifiers());
                    event_msg->Append_Int_Arg(m_currMouse.wheel_pos);
                case 8:
                    event_msg = g_theMessageStream->Append_Message(GameMessage::MSG_RAW_MOUSE_LEFT_DRAG);
                    event_msg->Append_Pixel_Arg(m_currMouse.pos);
                    event_msg->Append_Int_Arg(g_theKeyboard->Get_Modifiers());
                default:
                    break;
            }

            switch (m_currMouse.middle_event) {
                case 9:
                    event_msg = g_theMessageStream->Append_Message(GameMessage::MSG_RAW_MOUSE_MIDDLE_BUTTON_DOWN);
                    event_msg->Append_Pixel_Arg(m_currMouse.pos);
                    event_msg->Append_Int_Arg(g_theKeyboard->Get_Modifiers());
                    event_msg->Append_Int_Arg(m_currMouse.wheel_pos);
                case 10:
                    event_msg = g_theMessageStream->Append_Message(GameMessage::MSG_RAW_MOUSE_MIDDLE_BUTTON_UP);
                    event_msg->Append_Pixel_Arg(m_currMouse.pos);
                    event_msg->Append_Int_Arg(g_theKeyboard->Get_Modifiers());
                    event_msg->Append_Int_Arg(m_currMouse.wheel_pos);
                case 11:
                    event_msg = g_theMessageStream->Append_Message(GameMessage::MSG_RAW_MOUSE_MIDDLE_DOUBLE_CLICK);
                    event_msg->Append_Pixel_Arg(m_currMouse.pos);
                    event_msg->Append_Int_Arg(g_theKeyboard->Get_Modifiers());
                    event_msg->Append_Int_Arg(m_currMouse.wheel_pos);
                case 12:
                    event_msg = g_theMessageStream->Append_Message(GameMessage::MSG_RAW_MOUSE_MIDDLE_DRAG);
                    event_msg->Append_Pixel_Arg(m_currMouse.pos);
                    event_msg->Append_Int_Arg(g_theKeyboard->Get_Modifiers());
                default:
                    break;
            }

            switch (m_currMouse.right_event) {
                case 13:
                    event_msg = g_theMessageStream->Append_Message(GameMessage::MSG_RAW_MOUSE_RIGHT_BUTTON_DOWN);
                    event_msg->Append_Pixel_Arg(m_currMouse.pos);
                    event_msg->Append_Int_Arg(g_theKeyboard->Get_Modifiers());
                    event_msg->Append_Int_Arg(m_currMouse.wheel_pos);
                case 14:
                    event_msg = g_theMessageStream->Append_Message(GameMessage::MSG_RAW_MOUSE_RIGHT_BUTTON_UP);
                    event_msg->Append_Pixel_Arg(m_currMouse.pos);
                    event_msg->Append_Int_Arg(g_theKeyboard->Get_Modifiers());
                    event_msg->Append_Int_Arg(m_currMouse.wheel_pos);
                case 15:
                    event_msg = g_theMessageStream->Append_Message(GameMessage::MSG_RAW_MOUSE_RIGHT_DOUBLE_CLICK);
                    event_msg->Append_Pixel_Arg(m_currMouse.pos);
                    event_msg->Append_Int_Arg(g_theKeyboard->Get_Modifiers());
                    event_msg->Append_Int_Arg(m_currMouse.wheel_pos);
                case 16:
                    event_msg = g_theMessageStream->Append_Message(GameMessage::MSG_RAW_MOUSE_RIGHT_DRAG);
                    event_msg->Append_Pixel_Arg(m_currMouse.pos);
                    event_msg->Append_Int_Arg(g_theKeyboard->Get_Modifiers());
                default:
                    break;
            }

            if (m_currMouse.wheel_delta != 0) {
                GameMessage *wheel_msg = g_theMessageStream->Append_Message(GameMessage::MSG_RAW_MOUSE_WHEEL);
                wheel_msg->Append_Pixel_Arg(m_currMouse.pos);
                wheel_msg->Append_Int_Arg(m_currMouse.wheel_delta / 120);
                wheel_msg->Append_Int_Arg(g_theKeyboard->Get_Modifiers());
            }
        }
    }
}

/**
 * @brief Sets the mouse position.
 *
 * 0x00403B40
 */
void Mouse::Set_Position(int x, int y)
{
    m_currMouse.pos.x = x;
    m_currMouse.pos.y = y;
}

/**
 * @brief Sets the limits on what mouse coordinates are valid based on current resolution.
 *
 * 0x00403B40
 */
void Mouse::Set_Mouse_Limits()
{
    // TODO Requires DisplayClass
#ifndef THYME_STANDALONE
    Call_Method<void, Mouse>(0x00403B60, this);
#elif 0
    m_minX = 0;
    m_minY = 0;

    if (g_theDisplay) {
        m_maxX = g_theDisplay->Get_Width();
        m_maxY = g_theDisplay->Get_Height();
    }
#endif
}

/**
 * @brief Causes mouse to update its configuration for the current resolution settings.
 *
 * 0x00403090
 */
void Mouse::Notify_Resolution_Change()
{
    // TODO Requires FontLibrary
#ifndef THYME_STANDALONE
    Call_Method<void, Mouse>(0x00403090, this);
#elif 0
    if (m_tooltipDisplayString != nullptr) {
        g_theDisplayStringManger->Free_Display_String(m_tooltipDisplayString);
    }

    m_tooltipDisplayString = g_theDisplayStringManger->New_Display_String();

    // FontDesc font;
    AsciiString font_name;
    int font_size;
    bool font_bold;

    if (g_theGlobalLanguage != nullptr && g_theGlobalLanguage->Tooltip().Name().Is_Not_Empty()) {
        font_bold = g_theGlobalLanguage->Tooltip().Bold();
        font_size = g_theGlobalLanguage->Adjust_Font_Size(g_theGlobalLanguage->Tooltip().Point_Size());
        font_name = g_theGlobalLanguage->Tooltip().Name();
    } else {
        font_bold = m_tooltipFontIsBold;
        font_size = g_theGlobalLanguage->Adjust_Font_Size(m_tooltipFontSize);
        font_name = m_tooltipFontName;
    }

    m_tooltipDisplayString->Set_Font(g_theFontLibrary->Get_Font(font_name, font_size, font_bold));
    m_tooltipDisplayString->Set_Word_Wrap(TOOLTIP_WRAP);
#endif
}

/**
 * @brief Updates mouse's internal data state.
 */
void Mouse::Update_Mouse_Data()
{
    static bool _busy;

    if (_busy) {
        m_deadInputFrame = m_inputFrame;
        return;
    }

    _busy = true;
    int8_t tmp;
    int events = 0;

    do {
        do {
            tmp = Get_Mouse_Event(&m_mouseEvents[events], 1);
        } while (tmp == -1);

        ++events;
    } while (tmp != 0 && events <= 255);

    _busy = false;

    if (events > 0) {
        unkInt3 = events - 1;
    } else {
        unkInt3 = 0;
    }

    if (events != 0) {
        m_deadInputFrame = m_inputFrame;
    }
}

/**
 * @brief Processes a given mouse event in the event buffer.
 *
 * 0x004024E0
 */
void Mouse::Process_Mouse_Event(int event_num)
{
    m_currMouse.left_event = 0;
    m_currMouse.right_event = 0;
    m_currMouse.middle_event = 0;
    m_currMouse.wheel_delta = 0;
    m_currMouse.wheel_pos = m_mouseEvents[event_num].wheel_pos;

    if (event_num == 0) {
        if (m_currMouse.left_state != 0) {
            if (m_prevMouse.left_event == 5 || m_prevMouse.left_event == 8) {
                m_currMouse.left_event = 8;
            }
        }

        if (m_currMouse.right_state != 0) {
            if (m_prevMouse.right_event == 13 || m_prevMouse.right_event == 16) {
                m_currMouse.right_event = 16;
            }
        }

        if (m_currMouse.middle_state != 0) {
            if (m_prevMouse.middle_event == 9 || m_prevMouse.middle_event == 12) {
                m_currMouse.middle_event = 12;
            }
        }
    }

    Move_Mouse(m_mouseEvents[event_num].pos.x, m_mouseEvents[event_num].pos.y, m_inputMovesAbsolute);
    m_currMouse.wheel_delta += m_mouseEvents[event_num].wheel_delta;

    // Process the states of each mouse button (assumes 3 buttons).
    if (m_mouseEvents[event_num].left_frame != 0) {
        if (m_currMouse.left_state != m_mouseEvents[event_num].left_state) {
            switch (m_mouseEvents[event_num].left_state) {
                case 1:
                    m_currMouse.left_event = 5;
                    m_currMouse.left_state = 1;
                    m_currMouse.left_frame = m_inputFrame;
                    break;
                case 2:
                    m_currMouse.left_event = 7;
                    m_currMouse.left_state = 2;
                    m_currMouse.left_frame = m_inputFrame;
                    break;
                default:
                    m_currMouse.left_event = 6;
                    m_currMouse.left_state = 0;
                    m_currMouse.left_frame = m_inputFrame;
                    break;
            }
        }
    } else if (m_currMouse.left_state != 0) {
        if (m_prevMouse.left_event == 5 || m_prevMouse.left_event == 8) {
            m_currMouse.left_event = 8;
        }
    }

    if (m_mouseEvents[event_num].right_frame != 0) {
        if (m_currMouse.right_state != m_mouseEvents[event_num].right_state) {
            switch (m_mouseEvents[event_num].right_state) {
                case 1:
                    m_currMouse.right_event = 13;
                    m_currMouse.right_state = 1;
                    m_currMouse.right_frame = m_inputFrame;
                    break;
                case 2:
                    m_currMouse.right_event = 15;
                    m_currMouse.right_state = 2;
                    m_currMouse.right_frame = m_inputFrame;
                    break;
                default:
                    m_currMouse.right_event = 14;
                    m_currMouse.right_state = 0;
                    m_currMouse.right_frame = m_inputFrame;
                    break;
            }
        }
    } else if (m_currMouse.right_state != 0) {
        if (m_prevMouse.right_event == 13 || m_prevMouse.right_event == 16) {
            m_currMouse.right_event = 16;
        }
    }

    if (m_mouseEvents[event_num].middle_frame != 0) {
        if (m_currMouse.middle_state != m_mouseEvents[event_num].middle_state) {
            switch (m_mouseEvents[event_num].middle_state) {
                case 1:
                    m_currMouse.middle_event = 9;
                    m_currMouse.middle_state = 1;
                    m_currMouse.middle_frame = m_inputFrame;
                    break;
                case 2:
                    m_currMouse.middle_event = 11;
                    m_currMouse.middle_state = 2;
                    m_currMouse.middle_frame = m_inputFrame;
                    break;
                default:
                    m_currMouse.middle_event = 10;
                    m_currMouse.middle_state = 0;
                    m_currMouse.middle_frame = m_inputFrame;
                    break;
            }
        }
    } else if (m_currMouse.middle_state != 0) {
        if (m_prevMouse.middle_event == 9 || m_prevMouse.middle_event == 12) {
            m_currMouse.middle_event = 12;
        }
    }

    m_currMouse.delta_pos.x = m_currMouse.pos.x - m_prevMouse.pos.x;
    m_currMouse.delta_pos.y = m_currMouse.pos.y - m_prevMouse.pos.y;
    memcpy(&m_prevMouse, &m_currMouse, sizeof(m_prevMouse));
}

/**
 * @brief Moves the mouse position.
 *
 * 0x00402450
 */
void Mouse::Move_Mouse(int x, int y, int absolute)
{
    if (absolute) {
        m_currMouse.pos.x = x;
        m_currMouse.pos.y = y;
    } else {
        m_currMouse.pos.x += x;
        m_currMouse.pos.y += y;
    }

    m_currMouse.pos.x = Clamp(m_currMouse.pos.x, m_minX, m_maxX);
    m_currMouse.pos.y = Clamp(m_currMouse.pos.y, m_minY, m_maxY);
}
