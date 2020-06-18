for line in io.lines() do
    io.write('"', line, [[\n"]], '\n')
end
os.exit()