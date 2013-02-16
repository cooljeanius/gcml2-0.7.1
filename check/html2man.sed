#
#  gcml2 -- an implementation of Eric Raymond's CML2 in C
#  Copyright (C) 2000-2002 Greg Banks
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Library General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Library General Public License for more details.
#
#  You should have received a copy of the GNU Library General Public
#  License along with this library; if not, write to the Free
#  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
s|^\.|\\\&.|g
s|\.|\\.|g
s|-|\\-|g
s|<dl>|.in +4m|g
s|</dl>|.in -4m|g
s|<dt>\(.*\)$|.in -4m\
\1\
.br|g
s|^<dd>$|.in +4m|
s|<a[ 	]\+name="[^"]*">||g
s|<a[ 	]\+href="[^"]*">|\\fB|g
s|</a>|\\fP|g
s|<b>|\\fB|g
s|</b>|\\fP|g
s|<i>|\\fI|g
s|</i>|\\fP|g
s|<span[ 	]\+class="keyword">|\\fB|g
s|<span[ 	]\+class="code">|\\fI|g
s|</span>|\\fP|g
/<pre[ 	]\+class="example">/,/<\/pre>/s/\\$/\\\\/
/<pre[ 	]\+class="example">/,/<\/pre>/s/$/\
.br/
s|<pre[ 	]\+class="example">|.in +4m\
\\fI|g
s|</pre>|\\fR\
.in -4m|g
s|^[ 	]*<ul>|.in +4m|g
s|^[ 	]*</ul>|.in -4m|g
s|^[ 	]*<li>|.IP \\(bu\
|
s|<p>|\
|
/<p[ 	]\+class="emphasis">/,/<\/p>/s|</p>|\\fP|
s|<p[ 	]\+class="emphasis">|\
\\fB|
s|</p>||
