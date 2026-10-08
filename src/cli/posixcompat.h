/*
 * Small POSIX/Windows compatibility shim for the CLI (ocg-*).
 *
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

#pragma once

#ifdef _WIN32
#include <io.h>
#include <process.h>
#define ocg_read _read
#define ocg_write _write
#define ocg_close _close
#define ocg_getpid _getpid
#define OCG_STDIN_FILENO 0
#define OCG_STDOUT_FILENO 1
#else
#include <unistd.h>
#define ocg_read ::read
#define ocg_write ::write
#define ocg_close ::close
#define ocg_getpid ::getpid
#define OCG_STDIN_FILENO STDIN_FILENO
#define OCG_STDOUT_FILENO STDOUT_FILENO
#endif
