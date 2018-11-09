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

import urllib

UNICODE_VERSION = '11.0.0'
URL_PREFIX = 'https://www.unicode.org/Public/%s/ucd/' % UNICODE_VERSION

FILES = [
    'DerivedCoreProperties.txt',
    'DerivedNormalizationProps.txt',
    'PropList.txt',
    'UnicodeData.txt',
]


if __name__ == '__main__':
    for filename in FILES:
        stream = urllib.urlopen(URL_PREFIX + filename)
        content = stream.read()
        stream.close()
        with open('src/third_party/unicode_data/%s' % filename, 'w') as out:
            out.write(content)
