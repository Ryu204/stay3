local component = {}
component.__index = component

function component:new()
	local obj = setmetatable({}, self)
	obj._entity = "null"
	return obj
end

function component:_on_attached(entity)
	self._entity = entity
end

return component
