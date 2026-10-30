"""Generate original, deterministic MIDI ideas, not artist melodies or synth patches.
Run with Python 3. Outputs are committed; Python is not needed to run the plugin.
Genre labels are creative starting points, not rigid musical classifications.
"""
from pathlib import Path
import xml.etree.ElementTree as ET
import hashlib
import json
from functools import lru_cache

ROOT = Path(__file__).resolve().parents[1]
# name, tempo, scale, rhythm family. Tempos are editable starting suggestions.
PSY = [
 ('PSYTRANCE',142,1,0),('PSYTRANCE CLASSIC',148,8,0),
 ('PSYTRANCE PROGRESSIVE',136,1,1),('PSYTRANCE FULL-ON',145,2,0),
 ('PSYTRANCE MORNING',144,7,2),('PSYTRANCE NIGHT',148,8,3),
 ('PSYTRANCE TWILIGHT',150,2,3),('PSYTRANCE GOA',145,2,2),
 ('PSYTRANCE NEO-GOA',148,2,4),('PSYTRANCE DARKPSY',158,8,3),
 ('PSYTRANCE FOREST',152,8,5),('PSYTRANCE DARK FOREST',156,11,5),
 ('PSYTRANCE HI-TECH',185,2,6),('PSYTRANCE PSYCORE',215,11,6),
 ('PSYTRANCE ZENONESQUE',136,8,5),('PSYTRANCE PSYTECH',130,7,1),
 ('PSYTRANCE SUOMI',148,10,4),('PSYTRANCE PSYDUB',90,7,7),
 ('PSYTRANCE PSYBIENT',84,1,7),('PSYTRANCE PSYBREAKS',138,5,8),
 ('PSYTRANCE HYPNOTIC',146,8,5),('PSYTRANCE RAW',140,1,4),
 ('PSYTRANCE TRIBAL',138,5,9),('PSYTRANCE NITZHONOT',148,2,2),
]
OTHER = [
 ('TECHNO',136,1,4),('TECHNO HYPNOTIC',138,8,5),('TECHNO MELODIC',124,1,2),
 ('TECHNO HARD',150,8,3),('TECHNO ACID',138,7,6),('HOUSE',124,7,1),
 ('HOUSE DEEP',120,7,7),('HOUSE TECH',128,5,1),('HOUSE PROGRESSIVE',126,1,2),
 ('HOUSE AFRO',122,5,9),('TRANCE',138,2,2),('BREAKBEAT',132,5,8),
 ('DRUM AND BASS',174,1,8),('DUBSTEP',140,8,7),
]
SCALES = {1:[0,2,3,5,7,8,10],2:[0,2,3,5,7,8,11],5:[0,3,5,7,10],7:[0,2,3,5,7,9,10],8:[0,1,3,5,7,8,10],10:[0,2,4,5,7,9,10],11:[0,1,3,5,6,8,10]}
FAMILIES = [('PIANO','BASS',0),('PIANO','LEAD',0),('PIANO','ARP',0),('PIANO','CHORDS',0),('PIANO','PAD',0),('GUITAR','LEAD',1),('GUITAR','ARP',1),('GUITAR','CHORDS',1),('BASS 4','BASS',2),('BASS 5','BASS',3)]
NAMES={'BASS':['Rolling Pulse','Octave Engine','Offbeat Orbit','Broken Drive'], 'LEAD':['Spiral Signal','Night Call','Acid Answer','Fractal Flight'], 'ARP':['Crystal Steps','Mirror Circuit','Orbit Sequence','Rising Geometry'], 'CHORDS':['Minor Motion','Suspended Space','Modal Chords','Stab Exchange'], 'PAD':['Distant Horizon','Slow Bloom','Twilight Air','Floating Resolve']}
MASKS=['0111011101110111','0010001000100010','1011011010110110','0111011101101110','1010101101011010','0101101100101101','1101011110101101','1000001010000100','1001001010010110','101101101101']
MOTIFS=[[0,0,4,0,0,2,0,6],[0,4,7,4,0,2,7,6],[0,1,0,4,2,0,6,4],[0,4,2,6,1,5,3,7]]
KEYS=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B']
TUNING=[40,45,50,55,59,64]

def fold(note,low,high):
 while note<low: note+=12
 while note>high: note-=12
 return note

@lru_cache(None)
def shape(pcs):
 candidates=[]
 for window in range(10):
  frets=[]
  for open_note in TUNING:
   options=[f for f in range(13) if (f==0 or window<=f<=window+3) and (open_note+f)%12 in pcs]
   frets.append(min(options) if options else -1)
  notes=sorted(set(n+f for n,f in zip(TUNING,frets) if f>=0))
  if set(n%12 for n in notes)==set(pcs):candidates.append((sum(max(f,0) for f in frets)+5*frets.count(-1),notes,frets))
 if not candidates:return None
 _,notes,frets=min(candidates)
 return notes,frets

def attrs(**values):return {k:str(int(v) if isinstance(v,bool) else v) for k,v in values.items()}

def generate():
 bank=ET.Element('XChordsFactoryBank',version='1');catalog=[];fingerprints=set()
 for gi,(genre,tempo,scale_id,rhythm) in enumerate(PSY+OTHER):
  scale=SCALES[scale_id]
  for fi,(instrument,role,mode) in enumerate(FAMILIES):
   for variant in range(4):
    key=(gi*5+variant*3+fi)%12
    bpm=tempo+[-2,0,2,4][variant]
    title=f'{instrument} - {role} - {genre} - {NAMES[role][variant]}'
    identity=hashlib.sha256(title.encode()).hexdigest()[:20]
    preset=ET.SubElement(bank,'XChordsPreset',attrs(id=identity,name=title,instrument=instrument,role=role,genre=genre,bpm=bpm,key=KEYS[key],version=1,favourite=False))
    state=ET.SubElement(preset,'XChords',attrs(language=1,scale=1,formula=scale_id,root=key,instrument=mode,octave=3 if role=='BASS' else 5,custom=True,timbre=2 if role=='BASS' else 1 if role=='PAD' else 0,bpm=bpm,loop=True,velocity=96,pattern=1 if role in ('ARP','LEAD') else 0,duration=2,bassPattern=0))
    progression=ET.SubElement(state,'Progression')
    source=[];records=[];bars=4 if role in ('PAD','CHORDS') else 2
    if role in ('PAD','CHORDS'):
     roots=[[0,5,3,4],[0,3,6,4],[0,4,5,3],[0,6,3,4]][variant]
     for bar,degree in enumerate(roots):
      notes=[48+key+scale[(degree+d)%len(scale)]+12*((degree+d)//len(scale)) for d in (0,2,4)]
      if role=='PAD':notes.append(notes[0]+12)
      frets=[]
      if mode==1:
       found=shape(tuple(sorted(set(n%12 for n in notes))))
       if found:notes,frets=found
       else:
        # A monophonic guitar fragment is always reachable, but never advertise
        # it as a full chord: this branch must not occur for the chosen bank.
        raise ValueError(f'No guitar shape: {title} {notes}')
      positions=[(4,False)] if role=='PAD' else [(1,False),(.5,True),(.5,False),(1,False),(1,True)]
      for si,(beats,rest) in enumerate(positions):
       gate=1 if role=='PAD' else [.42,.55,.68,.8][variant]
       step=attrs(name='Rest' if rest else f'Chord {bar+1}',notes='' if rest else ','.join(map(str,notes)),beats=beats,gate=gate,rest=rest,velocity=78+variant*7+(bar%2)*5,instrument=mode,scale=True,formula=scale_id,root=notes[0]%12,octave=4,strum=mode==1 and variant%2==1,frets=','.join(map(str,frets)))
       ET.SubElement(progression,'Step',step);records.append(step)
      source+=notes
    else:
     # Rhythm families: rolling, offbeat, syncopated, broken and triplet grids.
     mask=MASKS[(rhythm+(variant if role!='BASS' else [0,0,1,3][variant]))%len(MASKS)]
     cells=12 if len(mask)==12 else 16;beat=4/cells
     base=24+key if role=='BASS' else 60+key if role=='LEAD' else 48+key
     low,high=(40,76) if mode==1 else (28,55) if mode==2 else (23,55) if mode==3 else (24,55) if role=='BASS' else (48,88)
     motif=MOTIFS[variant]
     for i in range(cells*bars):
      active=mask[(i+(gi%3 if role!='BASS' else 0))%cells]=='1'
      degree=motif[(i+(gi%7)+(fi if role!='BASS' else 0))%len(motif)]
      if role=='BASS':degree=0 if variant==0 else [0,4,7,4][(i//3+gi)%4] if variant==1 else (4 if i%7==0 else 0) if variant==2 else motif[(i//2+gi)%8]
      if role=='ARP':degree=[0,2,4,6,7,4,2,1][(i+variant+gi)%8]
      note=fold(base+scale[degree%len(scale)]+12*(degree//len(scale)),low,high)
      frets=[]
      if mode==1:
       options=[(note-n,s) for s,n in enumerate(TUNING) if 0<=note-n<=12];fret,string=min(options);frets=[-1]*6;frets[string]=fret
      velocity=76+((i*7+gi*3+variant*11+fi)%33)+(7 if i%4==1 else 0)
      step=attrs(name='Rest' if not active else f'{role.title()} {i+1:02}',notes=str(note) if active else '',beats=format(beat,'.12g'),gate=[.46,.6,.72,.86][variant] if role=='BASS' else [.55,.72,.84,.95][variant],rest=not active,velocity=velocity,instrument=mode,bassPattern=0,root=note%12,scale=True,formula=scale_id,octave=note//12,frets=','.join(map(str,frets)))
      ET.SubElement(progression,'Step',step);records.append(step)
      if active:source.append(note)
    source=sorted(set(source));state.set('notes',','.join(map(str,source)))
    state.set('octave',str(max(2,min(6,source[0]//12))))
    # Guitar preview/reference is a scale fragment; progression stores exact frets.
    total=sum(float(s['beats']) for s in records)
    assert source and len(records)<=128 and abs(total-bars*4)<1e-8
    signature=json.dumps([mode,bpm,[(s['notes'],s['beats'],s['gate'],s['velocity'],s['rest']) for s in records]])
    assert signature not in fingerprints, f'Duplicate musical preset: {title}'
    fingerprints.add(signature)
    preset.set('bars',str(bars));preset.set('description',f'{KEYS[key]} · {bpm} BPM · {bars} bars · {role.lower()} MIDI idea')
    catalog.append(dict(name=title,instrument=instrument,role=role,genre=genre,bpm=bpm,key=KEYS[key],bars=bars,id=identity))
 (ROOT/'resources').mkdir(exist_ok=True)
 ET.indent(bank)
 ET.ElementTree(bank).write(ROOT/'resources/factory-presets.xml',encoding='utf-8',xml_declaration=True)
 (ROOT/'resources/factory-catalog.json').write_text(json.dumps(catalog,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
 print(f'{len(catalog)} original presets; {len(PSY)*len(FAMILIES)*4} psy-family; {len(fingerprints)} unique musical signatures')

if __name__=='__main__':generate()
