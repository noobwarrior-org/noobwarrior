/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: IdeToolboxHandler.h
// Started by: Hattozo
// Started on: 6/6/2026
// Description: Legacy Studio toolbox endpoints: /IDE/Toolbox/Items.aspx (JSON) and /IDE/ClientToolbox.aspx (HTML).
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>

namespace NoobWarrior {
class IdeToolboxHandler : public Handler {
public:
    IdeToolboxHandler(EmuDbManager *dbm);
    void OnRequest(evhttp_request *req, void *userdata) override;
private:
    void HandleItemsJson(evhttp_request *req);
    void HandleClientToolboxHtml(evhttp_request *req);

    EmuDbManager *mEmuDbManager;
};
}
