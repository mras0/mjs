#!/usr/bin/env python

import xml.etree.ElementTree as ET
from datetime import datetime

# The UCD can be downloaded at (for v. 11.0.0): https://www.unicode.org/Public/11.0.0/ucdxml/ucd.all.flat.zip
filename = 'ucd.all.flat.xml'

ns = {'ucd': 'http://www.unicode.org/ns/2003/ucd/1.0'}
tree = ET.parse(filename)
root = tree.getroot()
repertoire = root.find('ucd:repertoire', ns)
chars = repertoire.findall('ucd:char', ns)

classifications = {
    'format': {
        'categories': ['Cf'],
        'description': 'Format-Control Character'
    },
    'whitespace': {
        'categories': ['Zs'],
        'description': 'White Space'
    },
    'id_start': {
        'categories': ['Lu', 'Ll', 'Lt', 'Lm', 'Lo', 'Nl'],
        'description': 'Identifier Start'
    },
    'id_part': {
        'categories': ['Mn', 'Mc', 'Nd', 'Pc'],
        'description': 'Identifier Part'
    },
    'other': {
        'categories': [], # Every other category not mentioned above
        'description': 'Other'
    },
}


def translate_gc(gc):
    for c in classifications:
        if gc in classifications[c]['categories']:
            return c
    return 'other'

run_cp = -1
run_gc = ''
last_cp = -1

runs = []

for ch in chars:
    if not 'cp' in ch.attrib:
        continue
    cp = int(ch.attrib['cp'], 16) # Code point
    gc = translate_gc(ch.attrib['gc']) # General class
    if run_gc != gc or cp != last_cp + 1:
        if run_cp >= 0:
            runs.append((run_cp, run_gc))
        run_cp = cp
        run_gc = gc
    last_cp = cp

runs.append((run_cp, run_gc))
runs.append((last_cp, run_gc))

INDENT = 4 * ' '

def format_list(type_and_name, l, f):
    res = 'static constexpr const ' + type_and_name + '[%d] = {\n' % len(l);
    line = ''
    for x in l:
        if len(line) == 0:
            line = INDENT
        elif len(line) >= 71:
            res += line + '\n'
            line = INDENT
        else:
            line += ' '
        line += '%s,' % f(x)
    if len(line) > 0:
        res += line + '\n'
    res += '};'
    return res

now = datetime.utcnow().replace(microsecond = 0)
print("""#ifndef MJS_UNICODE_DATA_H
#define MJS_UNICODE_DATA_H

// Auto-generated file - do not modify by hand
// Generated at %s
// Based on the Unicode Character Databse in XML (https://www.unicode.org), version: %s

#include <cstdint>

namespace mjs::unicode {

enum class classification : std::uint8_t {""" % (now.isoformat(), root.find('ucd:description', ns).text))
for c in sorted(classifications.keys()):
    print('%s%-12s // %s' % (INDENT, c+',', classifications[c]['description']))
print('};')
print('')
print(format_list('uint32_t classification_run_start', runs, lambda x: '0x%05x' % x[0]))
print('')
print(format_list('classification classification_run_type', runs, lambda x: 'classification::%-11s' % x[1]))
print("""
} // namespace mjs::unicode

#endif""")
