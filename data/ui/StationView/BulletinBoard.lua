-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local Engine = import("Engine")
local Game = import("Game")
local SpaceStation = import("SpaceStation")
local ChatForm = import("ChatForm")
local utils = import("utils")

local ui = Engine.ui

local bulletinBoard = function (args)
    local station = Game.player:GetDockedWith()

	local bbTable =
		ui:Table()
			:SetMouseEnabled(true)

	bbTable:AddRows(utils.build_array(utils.map(function (k,v) return k,{v[1]} end, ipairs(SpaceStation.adverts[station]))))

	bbTable.onRowClicked:Connect(function (row)
		print(SpaceStation.adverts[station][row+1][1])
		local form = ChatForm.New()
		SpaceStation.adverts[station][row+1][2](form, row+1, 0)
		ui:NewLayer(
			ui:ColorBackground(0,0,0,0.5,
				ui:Align("MIDDLE",
					ui:Background(
						form:BuildWidget()
					)
				)
			)
		)
	end)

	return bbTable
end

return bulletinBoard
