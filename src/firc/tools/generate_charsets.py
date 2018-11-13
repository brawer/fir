#!/usr/bin/env python
# coding: utf-8
#
# Copyright 2018 by Sascha Brawer <sascha@brawer.ch>
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an “AS IS” BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import re


def parse_properties(text):
    r = re.compile(
        r'^([0-9A-F]+)(?:\.\.([0-9A-F]+))?\s*;\s*([_A-Za-z0-9]+)\s*[;#]?.*$')
    result = {}
    for line in text.splitlines():
        match = r.match(line)
        if not match:
            continue
        start, end, prop = match.groups()
        if end is None:
                end = start
        result.setdefault(prop, []).append((int(start, 16), int(end, 16)))
    patched = {}
    for key, value in result.items():
        range_start, range_end = None, None
        ranges = []
        for s, e in sorted(value):
            if range_start is None:
                range_start, range_end = s, e
            elif range_end + 1 == s:
                range_end = e
            else:
                ranges.append((range_start, range_end))
                range_start, range_end = s, e
        ranges.append((range_start, range_end))
        patched[key] = ranges
    return patched


def parse_ccc(text):
    result = {}
    for line in text.splitlines():
        cols = line.split(';')
        char, klass = int(cols[0], 16), int(cols[3])
        if klass != 0:
            result[char] = klass
    return result


def parse_decompositions(text):
    nfd, nfkd = {}, {}
    for line in text.splitlines():
        cols = line.split(';')
        char, decomp = int(cols[0], 16), cols[5]
        if decomp:
            decomp_chars = [int(x, 16) for x in decomp.split('>')[-1].split()]
            nfkd[char] = decomp_chars
            if not decomp.startswith('<'):
                nfd[char] = decomp_chars
    return nfd, nfkd


def write_charset(name, charset, out):
     out.write('\nconst llvm::sys::UnicodeCharSet %s({\n' % name)
     for i, (start, end) in enumerate(charset):
         if i % 4 == 0:
             out.write('    ')
         out.write('{0x%05X, 0x%05X}' % (start, end))
         if i != len(charset) - 1:
             out.write(',')
         if i % 4 == 3:
             out.write('\n')
         else:
             out.write(' ')
     out.write('\n});\n')


def decompose(c, nfkd):
    result = []
    if c in nfkd:
        for d in nfkd[c]:
            result.extend(decompose(d, nfkd))
    else:
        result.append(c)
    return result


def write_decompositions(nfkd, id_charset, out):
    data = []
    out.write('\nconst uint32_t Lexer::NumCharDecompositions = %d;\n\n' %
              len(nfkd))
    out.write(
        '\nconst Lexer::CharDecomposition Lexer::CharDecompositions[] = {\n')
    for char in sorted(nfkd.keys()):
        if char not in id_charset:
            continue
        decomposed = decompose(char, nfkd)
        if len(decomposed) > 1:
            offset = str(len(data))
            data.extend(decomposed)
        else:
            offset = '0x%05X' % decomposed[0]
        out.write('    { 0x%05X, %d, %s },\n' %
                  (char, len(decomposed), offset))
    out.write('\n};\n')
    out.write(
        '\nconst uint32_t Lexer::CharDecompositionData[] = {\n')
    for i, d in enumerate(data):
        if i % 8 == 0:
            out.write('    ')
        out.write('0x%05X,' % d)
        out.write('\n' if i % 8 == 7 else ' ')
    out.write('\n};\n')


def write_ccc(ccc, out):
    out.write('\nconst uint16_t Lexer::NumCCCEntries = %d;\n\n' %
              len(ccc))
    out.write('\nconst Lexer::CCCEntry Lexer::CCCEntries[] = {\n')
    for char, klass in sorted(ccc.items()):
        out.write('    { 0x%05X, %d},\n' % (char, klass))
    out.write('\n};\n')


def write_compositions(compositions, out):
    out.write('\nconst uint32_t Lexer::NumCharCompositions = %d;\n\n' %
              len(compositions))
    out.write(
        '\nconst Lexer::CharComposition Lexer::CharCompositions[] = {\n')
    for (a, b), c in sorted(compositions.items()):
        out.write('    { 0x%05X, 0x%05X, 0x%05X },\n' % (a, b, c))
    out.write('\n};\n')


def read_ucd():
    with open('src/third_party/unicode_data/UnicodeData.txt', 'r') as f:
        content = f.read()
        return parse_ccc(content), parse_decompositions(content)


def read_properties(propfile):
    with open('src/third_party/unicode_data/%s' % propfile, 'r') as f:
        return parse_properties(f.read())


def build_charset(ranges):
    c = set()
    for start, end in ranges:
        c.update({ch for ch in range(start, end + 1)})
    return c


def build_canonical_composition(nfd, excluded, xid_chars):
    composition = {}
    total, s1, s2 = set(), set(), set()
    for c, decomp in nfd.items():
        if (c in excluded) or (c not in xid_chars):
            continue
        assert len(decomp) == 2, (c, decomp)
        composition[(decomp[0], decomp[1])] = c
    return composition


with open('src/firc/generated_charsets.cc', 'w') as out:
    ccc, (nfd, nfkd) = read_ucd()

    dprops = read_properties('DerivedCoreProperties.txt')
    xid_chars = build_charset(dprops['XID_Start'])
    xid_chars.update(build_charset(dprops['XID_Continue']))

    props = read_properties('PropList.txt')
    dnorm_props = read_properties('DerivedNormalizationProps.txt')
    compositions = build_canonical_composition(
        nfd,
        excluded=build_charset(dnorm_props['Full_Composition_Exclusion']),
        xid_chars=xid_chars)

    out.write('// Generated by src/firc/tools/generate_charsets.py\n\n')
    out.write('#include <llvm/Support/UnicodeCharRanges.h>\n')
    out.write('#include "firc/lexer.h"\n\n')
    out.write('namespace firc {\n')
    write_charset('Lexer::IDStartChars', dprops['XID_Start'], out)
    write_charset('Lexer::IDPartChars', dprops['XID_Continue'], out)
    write_charset('Lexer::WhitespaceChars', props['White_Space'], out)
    write_charset('Lexer::PossiblyNotNFKCChars', dnorm_props['NFKC_QC'], out)
    write_decompositions(nfkd, xid_chars, out)
    write_ccc(ccc, out)
    write_compositions(compositions, out)
    out.write('\n}  // namespace firc\n\n')
