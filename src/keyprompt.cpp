/*
 * This file is part of openconnect-gui.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "keyprompt.h"

static KeyPasswordPrompt g_prompt;

void set_key_password_prompt(KeyPasswordPrompt fn)
{
    g_prompt = std::move(fn);
}

bool ask_key_password(const QString& title, const QString& label, QString& out)
{
    if (!g_prompt) {
        return false;
    }
    return g_prompt(title, label, out);
}
