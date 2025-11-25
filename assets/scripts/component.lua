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

function component:start() end

function component:update(delta_time) end

function component:post_update(delta_time) end

function component:input() end

function component:on_destroy() end

return component
