-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local Engine = import("Engine")

local ui = Engine.ui

local shipRepairs = function (args)
    return ui:Label("ship repairs")
end

return shipRepairs
