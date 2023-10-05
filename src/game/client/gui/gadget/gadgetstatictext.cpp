/**
 * @file
 *
 * @author Jonathan Wilson
 *
 * @brief Static Text
 *
 * @copyright Thyme is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "gadgetstatictext.h"
#include "displaystring.h"
#include "gamewindow.h"
#include "gamewindowmanager.h"
#include "keyboard.h"

void Gadget_Static_Text_Set_Text(GameWindow *static_text, Utf16String text)
{
#ifdef GAME_DLL // temporary since we can't change the definition of Win_Send_System_Msg at this point and we can't cast a
                // pointer to an unsigned int on 64 bit
    g_theWindowManager->Win_Send_System_Msg(static_text, GGM_SET_LABEL, reinterpret_cast<unsigned int>(&text), 0);
#endif
}

void Gadget_Static_Text_Set_Font(GameWindow *static_text, GameFont *font)
{
    _TextData *data = static_cast<_TextData *>(static_text->Win_Get_User_Data());
    DisplayString *string = static_text->Win_Get_Instance_Data()->Get_Text_DisplayString();

    if (string != nullptr) {
        string->Set_Font(font);
    }

    DisplayString *tooltip_string = static_text->Win_Get_Instance_Data()->Get_Tooltip_DisplayString();

    if (tooltip_string != nullptr) {
        tooltip_string->Set_Font(font);
    }

    if (data != nullptr) {
        if (data->m_text != nullptr) {
            data->m_text->Set_Font(font);
        }
    }
}

WindowMsgHandledType Gadget_Static_Text_Input(
    GameWindow *static_text, unsigned int message, unsigned int data_1, unsigned int data_2)
{

    if (message != GWM_CHAR) {
        return MSG_IGNORED;
    }

    switch (data_1) {
        case Keyboard::KEY_TAB:
        case Keyboard::KEY_RIGHT:
        case Keyboard::KEY_DOWN:
            if ((data_2 & Keyboard::KEY_STATE_DOWN) != 0) {
                static_text->Win_Next_Tab();
            }

            return MSG_HANDLED;
        case Keyboard::KEY_UP:
        case Keyboard::KEY_LEFT:
            if ((data_2 & Keyboard::KEY_STATE_DOWN) != 0) {
                static_text->Win_Prev_Tab();
            }

            return MSG_HANDLED;
        default:
            return MSG_IGNORED;
    }
}

WindowMsgHandledType Gadget_Static_Text_System(
    GameWindow *static_text, unsigned int message, unsigned int data_1, unsigned int data_2)
{
#ifdef GAME_DLL
    return Call_Function<WindowMsgHandledType, GameWindow *, unsigned int, unsigned int, unsigned int>(
        PICK_ADDRESS(0x005A2DF0, 0x008DFAA7), static_text, message, data_1, data_2);
#else
    return MSG_IGNORED;
#endif
}
