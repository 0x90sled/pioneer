-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local Engine = import("Engine")
local Lang = import("Lang")
local Game = import("Game")
local ShipDef = import("ShipDef")
local Comms = import("Comms")

local InfoGauge = import("ui/InfoGauge")

local EquipmentTableWidgets = import("EquipmentTableWidgets")

local l = Lang.GetResource("ui-core")

local ui = Engine.ui


local equipmentMarket = function (args)
	local player = Game.player
	local station = player:GetDockedWith()

	local cargoGauge = InfoGauge.New({
		formatter = function (v)
			return string.format("%d/%dt", player.usedCargo, ShipDef[player.shipId].capacity-player.usedCapacity+player.usedCargo)
		end
	})

	local cashLabel = ui:Label("")

	local function updateStats ()
		cargoGauge:SetValue(player.usedCargo/(ShipDef[player.shipId].capacity-player.usedCapacity+player.usedCargo))
		cashLabel:SetText(string.format("$%.2f", player:GetMoney()))
	end
	updateStats()

	local stationTable, shipTable = EquipmentTableWidgets.Pair({
		station = station,

		isValidSlot = function (slot) return slot ~= "CARGO" end,

		onBuy = function (e)
			if station:GetEquipmentStock(e) <= 0 then
				Comms.message(l.ITEM_IS_OUT_OF_STOCK)
				return
			end

			-- XXX check slot capacity

			if player.freeCapacity <= 0 then
				Comms.Message(l.SHIP_IS_FULLY_LADEN)
				return
			end

			if player:GetMoney() < station:GetEquipmentPrice(e) then
				Comms.Message(l.YOU_NOT_ENOUGH_MONEY)
				return
			end

			assert(player:AddEquip(e) == 1)
			player:AddMoney(-station:GetEquipmentPrice(e))
			-- XXX remove from station stock

			updateStats()
		end,

		onSell = function (e)
			player:RemoveEquip(e)
			player:AddMoney(station:GetEquipmentPrice(e))
			-- XXX add to station stock

			updateStats()
		end,
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
					ui:HBox():PackEnd({
						"Cash: ",
						cashLabel
					}),
					ui:HBox():PackEnd({
						"Cargo space: ",
						cargoGauge
					})
				})
			})
end

return equipmentMarket
