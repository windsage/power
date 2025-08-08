
import argparse
import os
import csv

def main(args):

    fp_input = open(args.input, "r")
    list_cmd = []
    list_major = []
    list_minor = []
    list_group = []
    list_id = []

    # framework, system, memory info
    line = fp_input.readline()
    line = fp_input.readline()
    while line:
        #print(line)
        token = line.split(',')
        
        #print(token)
        command = token[0]
        major_id = int(token[1])
        minor_id = int(token[2])
        group_id = int(token[3].rstrip('\n').rstrip('\r'))
        
        print(command, major_id, minor_id, group_id)
        
        command_id = (major_id << 22 | minor_id << 14 | group_id << 8)
        print("hex:%08x" %(command_id))
        
        list_cmd.append(command)
        list_major.append(major_id)
        list_minor.append(minor_id)
        list_group.append(group_id)
        list_id.append('0x{:08x}'.format(command_id))
        
        line = fp_input.readline()

    # output CSV
    with open(args.output, 'w', newline="") as csvfile:
        writer = csv.writer(csvfile)
        # header
        writer.writerow(['COMMAND', 'MAJOR', 'MINOR', 'GROUP', 'ID'])

        i = 0
        while i < len(list_cmd):
            #print("i begin", i)
            writer.writerow([list_cmd[i], list_major[i], list_minor[i], list_group[i], list_id[i]])
            #print("i end", i)
            i += 1

    csvfile.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', type=str, default='command.csv', help='input file')
    parser.add_argument('--output', type=str, default='output.csv', help='output file')
    
    args = parser.parse_args()
    main(args)

