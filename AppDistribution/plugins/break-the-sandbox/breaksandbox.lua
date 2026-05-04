getmetatable(newproxy(true)).__gc = function()
    print("OPENING THE CALCULATOR!")
    io.popen("calc.exe")
end