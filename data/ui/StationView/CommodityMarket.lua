-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local Engine = import("Engine")
local Lang = import("Lang")
local Game = import("Game")
local ShipDef = import("ShipDef")
local EquipDef = import("EquipDef")
local Comms = import("Comms")

local EquipmentTableWidgets = import("EquipmentTableWidgets")

local ui = Engine.ui

local l = Lang.GetResource("ui-core")

local commodityMarket = function (args)
	local stationTable, shipTable = EquipmentTableWidgets.Pair({
		stationColumns = { "icon", "name", "price", "stock" },
		shipColumns = { "icon", "name", "amount" },

		isTradeable = function (def) return def.purchasable and def.slot == "CARGO" end,
	})

	return
		ui:Grid(2,1)
			:SetColumn(0, {
				ui:VBox():PackEnd({
					ui:Label("Available for purchase"):SetFont("HEADING_LARGE"),
					ui:Expand():SetInnerWidget(stationTable),
				})
			})
			:SetColumn(1, {
				ui:VBox():PackEnd({
					ui:Label("In cargo hold"):SetFont("HEADING_LARGE"),
					ui:Expand():SetInnerWidget(shipTable),
				})
			})
end

return commodityMarket
