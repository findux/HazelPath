of_log("example.lua calisiyor")

function calculate_feed_rate(base_feed_rate, multiplier)
    return base_feed_rate * multiplier
end

lua_result = calculate_feed_rate(cpp_feed_rate, 1.5)

of_log("Hesaplanan ilerleme hizi: " .. lua_result)