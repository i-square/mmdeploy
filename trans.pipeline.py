import json
import sys

f = open(sys.argv[1])
idx = int(sys.argv[2])
j = json.load(f)
t = j['pipeline']['tasks'][0]['transforms']
l = t[0]
if 'color_type' not in l:
    l['color_type'] = 'grayscale'

if idx >= 0:
    t.pop(idx)

# for lmk
p = j['pipeline']['tasks'][2]
#if p['module'] == 'mmpose':
#    para = p['params']
#    if para['unbiased']:
#        para['post_process'] = 'unbiased'

print(json.dumps(j))

#save_path = str(sys.argv[3])
#with open(save_path, "w") as fw:
#    json.dump(j, fw)
