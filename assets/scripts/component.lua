local component = {}
component.__index = component

function component:new()
	local obj = setmetatable({}, self)
	obj.entity = "null"
	return obj
end

function component:_on_attached(entity)
	self.entity = entity
	self.is_valid = true
end

function component:_on_detached()
	self.is_valid = false
end

function component:start() end

function component:update(delta_time) end

function component:post_update(delta_time) end

function component:input() end

function component:on_destroy() end

return component
