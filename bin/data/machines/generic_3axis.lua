-- Generic 3-axis reverse post-processor example.
-- Contract: convert_nc(nc_text) must return a Toolpath object.
-- A real machine profile can replace modal rules without changing C++.

local function copy_position(position)
    local result = AxisPosition.new(position.x, position.y, position.z)
    result.a = position.a
    result.b = position.b
    result.c = position.c
    return result
end

local function approximately(value, expected)
    return math.abs(value - expected) < 0.000001
end

function convert_nc(nc_text)
    local program = gpr.parse_with_source(nc_text)
    local path = Toolpath.new()
    path.machine_name = "generic_3axis"
    path.source_name = "editor"
    path.units = ToolpathUnits.Millimeters

    local current = AxisPosition.new(0.0, 0.0, 0.0)
    local absolute_mode = true
    local motion_mode = 0
    local feed_rate = 0.0
    path:set_current_position(current)

    for block_index = 0, program:num_blocks() - 1 do
        local block = program:get_block(block_index)
        if not block:is_deleted() then
            local next_position = copy_position(current)
            local has_axis = false
            local source_line = block:line_number()
            if source_line < 0 then
                source_line = block_index + 1
            end

            for chunk_index = 0, block:size() - 1 do
                local chunk = block:get_chunk(chunk_index)
                if chunk:type() == GprChunkType.WordAddress then
                    local word = string.upper(chunk:word())
                    local value = chunk:address():number()

                    if word == "G" then
                        if approximately(value, 0) then motion_mode = 0 end
                        if approximately(value, 1) then motion_mode = 1 end
                        if approximately(value, 20) then
                            path.units = ToolpathUnits.Inches
                        end
                        if approximately(value, 21) then
                            path.units = ToolpathUnits.Millimeters
                        end
                        if approximately(value, 90) then absolute_mode = true end
                        if approximately(value, 91) then absolute_mode = false end
                    elseif word == "X" then
                        next_position.x = absolute_mode and value or (current.x + value)
                        has_axis = true
                    elseif word == "Y" then
                        next_position.y = absolute_mode and value or (current.y + value)
                        has_axis = true
                    elseif word == "Z" then
                        next_position.z = absolute_mode and value or (current.z + value)
                        has_axis = true
                    elseif word == "A" then
                        next_position.a = absolute_mode and value or (current.a + value)
                        has_axis = true
                    elseif word == "B" then
                        next_position.b = absolute_mode and value or (current.b + value)
                        has_axis = true
                    elseif word == "C" then
                        next_position.c = absolute_mode and value or (current.c + value)
                        has_axis = true
                    elseif word == "F" then
                        feed_rate = value
                    end
                end
            end

            if has_axis then
                if motion_mode == 0 then
                    path:add_rapid(next_position, source_line, block:source())
                elseif motion_mode == 1 then
                    path:add_linear(next_position, feed_rate, source_line, block:source())
                end
                current = next_position
            end
        end
    end

    path:set_metadata("converter", "generic_3axis.lua")
    path:set_metadata("contract_version", "1")
    return path
end