#!/usr/bin/env python3
"""Import the user's Wikiti GLB into the MagicShelf SDK mesh, without Blender."""
import argparse, hashlib, json, struct
from pathlib import Path

def import_model(source, output):
    raw = source.read_bytes()
    if raw[:4] != b'glTF' or struct.unpack_from('<I', raw, 4)[0] != 2:
        raise ValueError('Expected a glTF 2 GLB')
    size, kind = struct.unpack_from('<II', raw, 12)
    if kind != 0x4e4f534a: raise ValueError('Missing GLB JSON')
    doc = json.loads(raw[20:20+size])
    offset = 20 + size
    length, kind = struct.unpack_from('<II', raw, offset)
    if kind != 0x004e4942: raise ValueError('Missing GLB binary')
    binary = raw[offset+8:offset+8+length]
    # This importer deliberately targets this model, not arbitrary GLTF scenes.
    if len(doc['meshes']) != 2 or 'wikiti' not in doc['asset']['extras']['author'].lower():
        raise ValueError('Expected Wikiti Pocket pikachu model')
    def accessor(index):
        a = doc['accessors'][index]; v = doc['bufferViews'][a['bufferView']]
        code = {5126:'f',5125:'I',5123:'H'}[a['componentType']]
        count = {'VEC3':3,'VEC2':2,'SCALAR':1}[a['type']]
        fmt = '<' + code*count; stride = v.get('byteStride', struct.calcsize(fmt))
        start = v.get('byteOffset',0) + a.get('byteOffset',0)
        return [struct.unpack_from(fmt,binary,start+i*stride) for i in range(a['count'])]
    primitive = doc['meshes'][0]['primitives'][0]
    positions = accessor(primitive['attributes']['POSITION'])
    normals = accessor(primitive['attributes']['NORMAL'])
    uv = accessor(primitive['attributes']['TEXCOORD_0'])
    # Ignore the Sketchfab presentation transforms. Model-local X is horizontal,
    # Z is up, and -Y faces the viewer. The imported body is 80 mm tall.
    low = [min(p[i] for p in positions) for i in range(3)]
    high = [max(p[i] for p in positions) for i in range(3)]
    scale = .080 / (high[2]-low[2]); cx = (low[0]+high[0])/2; cy = (low[2]+high[2])/2
    # The second primitive is the baked LCD: omit it and replace it with live pixels.
    records = []
    for (index,) in accessor(primitive['indices']):
        x,y,z = positions[index]; nx,ny,nz = normals[index]; u,v = uv[index]
        records.append(((x-cx)*scale,(z-cy)*scale,-y*scale,nx,nz,-ny,u,v))
    output.mkdir(parents=True,exist_ok=True)
    with (output/'device.mesh').open('wb') as out:
        out.write(b'DPH2'+struct.pack('<II',1,len(records))+struct.pack('<IIff',0,len(records),.75,0))
        for record in records: out.write(struct.pack('<8f',*record))
    texture = doc['textures'][doc['materials'][0]['pbrMetallicRoughness']['baseColorTexture']['index']]['source']
    view = doc['bufferViews'][doc['images'][texture]['bufferView']]
    start = view.get('byteOffset',0)
    (output/'body.png').write_bytes(binary[start:start+view['byteLength']])
    lcd = doc['accessors'][doc['meshes'][1]['primitives'][0]['attributes']['POSITION']]
    l,h = lcd['min'],lcd['max']
    info = {'source_sha256':hashlib.sha256(raw).hexdigest(),'source':source.name,'credits':doc['asset']['extras'],
        'triangles':len(records)//3,'scale':scale,'body_half_width':(high[0]-low[0])*scale/2,
        'screen':{'half_width':(h[0]-l[0])*scale/2,'half_height':(h[2]-l[2])*scale/2,
                  'center_y':((h[2]+l[2])/2-cy)*scale,'z':-l[1]*scale+.0001},'center_x':cx,'center_z':cy}
    (output/'model-info.json').write_text(json.dumps(info,indent=2)+'\n')
    print(json.dumps(info,indent=2))

if __name__ == '__main__':
    p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('--output',type=Path,default=Path('assets'))
    a=p.parse_args();import_model(a.source,a.output)
