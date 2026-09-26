from pathlib import Path
import json,xml.etree.ElementTree as ET,sys
import pcbnew
B=Path(__file__).resolve().parents[2]
data=json.loads((B/'hardware/design-data.json').read_text());u=next(x for x in data if x['ref']=='U3')
gpio_pad={1:35,3:34,26:11,27:12,4:26,34:6,18:30,19:31,23:37,21:33,22:36,16:27,17:28,25:10,33:9,13:16,14:13,32:8,36:4}
contract=json.loads((B/'firmware/hardware_contract.json').read_text());checks=[]
for row in contract['pins']:
 pad=gpio_pad[row['gpio']];actual=u['nets'].get(str(pad));checks.append({'gpio':row['gpio'],'module_pad':pad,'expected':row['net'],'actual':actual,'pass':actual==row['net']})
b=pcbnew.LoadBoard(str(B/'hardware/kicad/OpenDual-Reader.kicad_pcb'));fps={x.GetReference():x for x in b.GetFootprints()};bad=[];count=0;nc=[]
xml=ET.parse(B/'hardware/exports/schematic.net.xml')
for net in xml.findall('./nets/net'):
 name=net.attrib['name']
 for node in net.findall('node'):
  ref,pin=node.attrib['ref'],node.attrib['pin']
  if ref not in fps:
   if ref.startswith('PF'):continue
   bad.append([ref,pin,'missing footprint']);continue
  found=[x for x in fps[ref].Pads() if x.GetNumber()==pin]
  for pad in found:
   count+=1
   if name.startswith('unconnected-') and pad.GetNetname()==name:
    nc.append([ref,pin,'Schematic NC marker / PCB unique isolated net']);continue
   if pad.GetNetname()!=name:bad.append([ref,pin,name,pad.GetNetname()])
  if not found:bad.append([ref,pin,'missing pad'])
out={'gpio_checks':checks,'gpio_all_pass':all(x['pass'] for x in checks),'schematic_pcb_pads_checked':count,'intentional_no_connect_pads':nc,'schematic_pcb_mismatches':bad,'all_pass':all(x['pass'] for x in checks) and not bad}
(B/'hardware/exports/contract-validation.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
sys.exit(0 if out['all_pass'] else 1)
