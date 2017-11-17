filename = "DlRlcRetxStats.txt"

retxlines = []

with open(filename) as file:
    for line in file:
        if line[-2] == '1':
            retxlines.append(line)

print(retxlines)
