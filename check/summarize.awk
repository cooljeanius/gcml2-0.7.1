#!/usr/bin/awk -f
#
#  gcml2 -- an implementation of Eric Raymond's CML2 in C
#  Copyright (C) 2000-2001 Greg Banks
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
# $Id: summarize.awk,v 1.2 2002/09/01 09:10:34 gnb Exp $
#
function get_index(type,    i) {
    i = order[type]-1;
    if (i < 0) {
	i = ntypes;
	order[type] = i+1;
	name[i] = type;
	ntypes++;
    }
    return i;
}
BEGIN {
    last_type="";
    last_sym="";
    merge_mode = 0;
    ntypes = 0;
    # calls to get_index which force final output order
    get_index("missing-experimental-tag");
    get_index("spurious-experimental-tag");
    get_index("variant-experimental-tag");
    get_index("inconsistent-experimental-tag");
    get_index("missing-obsolete-tag");
    get_index("spurious-obsolete-tag");
    get_index("variant-obsolete-tag");
    get_index("inconsistent-obsolete-tag");
    get_index("spurious-dependencies");
    get_index("default-not-in-choices");
    get_index("empty-choices");
    get_index("nonliteral-define");
    get_index("unset-statement");
    get_index("different-banner");
    get_index("different-parent");
    get_index("overlapping-definitions");
    get_index("overlapping-mixed-definitions");
    get_index("primitive-in-root");
    get_index("undeclared-symbol");
    get_index("forward-compared-to-n");
    get_index("symbol-arch");
    get_index("forward-reference");
    get_index("forward-dependancy");
    get_index("undeclared-dependancy");
    get_index("symbol-like-literal");
    get_index("constant-symbol-misuse");
    get_index("constant-symbol-dependency");
    get_index("condition-loop");
    get_index("dependency-loop");
    get_index("unbalanced-endmenu");
    get_index("not-a-hex-number");
    get_index("missing-file");
    get_index("different-compound-type");
    get_index("unclassified warnings");
    get_index("unclassified errors");
    totali = get_index("total");
}
function get_nth_symbol(n) {
    s = $0;
    do {
        if (match(s, "CONFIG_[A-Za-z0-9_]*") == 0)
	    return "";
	v = substr(s, RSTART, RLENGTH);
	s = substr(s, RSTART+RLENGTH, length(s)-RSTART-RLENGTH);
    } while (--n);
    return v;
}
function get_symbol() {
    return get_nth_symbol(1);
}
function add_loc(i, sym, 	a, l) {
    if (split($0, a, ":") >= 3) {
	l = sprintf("%s:%d", a[2], a[3]);
	locs[i,sym,l]++;
    }
}
function got(type, sym,     i) {
    i = get_index(type);
    count[i]++;
    count[totali]++;
    if (sym != "") {
	vcount[i,sym]++;
    	if (verbose > 1)
 	    add_loc(i, sym);
    }
}

/^warning:.*:symbol CONFIG_[A-Za-z0-9_]* depends on CONFIG_EXPERIMENTAL but banner does not contain \(EXPERIMENTAL\)/ {
    got("missing-experimental-tag", get_nth_symbol(1));
    next;
}
/^warning:.*:banner for CONFIG_[A-Za-z0-9_]* contains \(EXPERIMENTAL\) but symbol does not depend on CONFIG_EXPERIMENTAL/ {
    got("spurious-experimental-tag", get_nth_symbol(1));
    next;
}
/^warning:.*:banner for CONFIG_[A-Za-z0-9_]* contains variant form of \(EXPERIMENTAL\)/ {
    got("variant-experimental-tag", get_symbol());
    next;
}
/^warning:symbol CONFIG_[A-Za-z0-9_]* is declared both with and without \(EXPERIMENTAL\)/ {
    got("inconsistent-experimental-tag", get_symbol());
    next;
}
/^warning:.*:symbol CONFIG_[A-Za-z0-9_]* depends on CONFIG_OBSOLETE but banner does not contain \(OBSOLETE\)/ {
    got("missing-obsolete-tag",get_nth_symbol(1));
    next;
}
/^warning:.*:banner for CONFIG_[A-Za-z0-9_]* contains \(OBSOLETE\) but symbol does not depend on CONFIG_OBSOLETE/ {
    got("spurious-obsolete-tag",get_nth_symbol(1));
    next;
}
/^warning:.*:banner for CONFIG_[A-Za-z0-9_]* contains variant form of \(OBSOLETE\)/ {
    got("variant-obsolete-tag",get_symbol());
    next;
}
/^warning:symbol CONFIG_[A-Za-z0-9_]* is defined both with and without \(OBSOLETE\)/ {
    got("inconsistent-obsolete-tag", get_symbol());
    next;
}
/^warning:.*:spurious dependencies after (bool|tristate)/ {
    got("spurious-dependencies", get_symbol());
    next;
}
/^warning:.*:default "[^"]*" not in choices list/ {
    got("default-not-in-choices");
    next;
}
/^warning:.*:empty "choices" statement/ {
    got("empty-choices");
    next;
}
/^warning:.*:symbol "CONFIG_[A-Za-z0-9_]*" defined to non-literal expression/ {
    got("nonliteral-define", get_nth_symbol(1));
    next;
}
/^warning:.*:unsupported "unset" statement used/ {
    got("unset-statement", "");
    next;
}
/^warning:.*:symbol "CONFIG_[A-Za-z0-9_]*" redefined with different banner/ {
    got("different-banner", get_symbol());
    next;
}
/^warning:.*:symbol "CONFIG_[A-Za-z0-9_]*" redefined with different parent/ {
    got("different-parent", get_symbol());
    next;
}
/^warning:.*:"CONFIG_[A-Za-z0-9_]*" has overlapping definitions/ {
    got("overlapping-definitions",get_symbol());
    next;
}
/^warning:.*:"CONFIG_[A-Za-z0-9_]*" has overlapping definitions as (derived symbol|symbol) and (derived symbol|symbol)/ {
    got("overlapping-mixed-definitions",get_symbol());
    next;
}
/^warning:.*:primitive symbol "CONFIG_[A-Za-z0-9_]*" in root menu/ {
    got("primitive-in-root", get_symbol());
    next;
}
/^warning:.*:symbol "CONFIG_[A-Za-z0-9_]*" used but not declared, defaults to ""/ {
    got("undeclared-symbol", get_symbol());
    next;
}
/^warning:.*:forward declared symbol "CONFIG_[A-Za-z0-9_]*" compared ambiguously to "n"/ {
    got("forward-compared-to-n", get_symbol());
    next;
}
/^warning:.*:undocumented symbol \$ARCH used/ {
    got("symbol-arch");
    next;
}
/^warning:.*:forward reference to "CONFIG_[A-Za-z0-9_]*"/ {
    got("forward-reference", get_symbol());
    next;
}
/^warning:.*:forward declared symbol "CONFIG_[A-Za-z0-9_]*" used in dependency list/ {
    got("forward-dependancy", get_nth_symbol(1));
    next;
}
/^warning:.*:symbol "CONFIG_[A-Za-z0-9_]*" used in dependency list but not declared/ {
    got("undeclared-dependancy", get_nth_symbol(1));
    next;
}
/^warning:.*:suspiciously symbol-like string literal "CONFIG_/ {
    got("symbol-like-literal", get_symbol());
    next;
}
/^warning:.*:misuse of constant symbol "CONFIG_[A-Za-z0-9_]*"/ {
    got("constant-symbol-misuse", get_symbol());
    next;
}
/^warning:.*:constant symbol "CONFIG_[A-Za-z0-9_]*" used in dependency list for "CONFIG_[A-Za-z0-9_]*"/ {
    got("constant-symbol-dependency", get_nth_symbol(2));
    next;
}
/^warning:.*:symbol "CONFIG_[A-Za-z0-9_]*" is conditional \(eventually\) on itself/ {
    got("condition-loop", get_symbol());
    next;
}
/^warning:.*:symbol "CONFIG_[A-Za-z0-9_]*" depends \(eventually\) on itself/ {
    got("dependency-loop", get_symbol());
    next;
}

/^warning:.*:location of (definition|previous definition)/ {
#     if (last_type != "")
#     	add_loc(last_type, last_sym);
#     last_type = "";
#     last_sym = "";
    next;
}
/^warning:.*:did you mean to use dep_/ {
    next;
}
/^warning:.*:overlap\([0-9]+\/[0-9]+\) is / {
    next;
}
/^warning:/ {
    got("unclassified warnings","");
    print
}


/^error:.*:unbalanced endmenu/ {
    if (!merge_mode) got("unbalanced-endmenu","");
    next;
}
/^error:.*:symbol `CONFIG_[A-Za-z0-9_]*' used but not declared or derived./ {
    got("undeclared-symbol", get_symbol());
    next;
}
/^error:.*:".*" is not a hexadecimal number/ {
    if (!merge_mode) got("not-a-hex-number","");
    next;
}
/^error:.*:can't open sourced file/ {
    if (!merge_mode) got("missing-file","");
    next;
}
/^error:.*:(menu|choice|comment) ".*" redefined as (menu|choice|comment)/ {
    if (!merge_mode) got("different-compound-type","");
    next;
}
/^error:.*:location of previous definition/ {
#     if (last_type != "")
#     	add_loc(last_type, last_sym);
#     last_type = "";
#     last_sym = "";
    next;
}
/^error:CML1 parser: [0-9][0-9]* errors/ {
    next;
}
/^error:/ {
    got("unclassified errors","");
    print
}
/^===== merge/ {
    merge_mode = 1;
}

END {
    for (i = 0 ; i < ntypes ; i++) {
    	if (count[i] == 0) continue;
    	printf "%-6d %s\n", count[i], name[i];
	if (verbose) {
#	    cmd = "sort -nr +0 -1";
	    cmd = "sort -k 1nr,2";
    	    if (verbose > 1) {
		for (k in locs) {
		    split(k,ka,SUBSEP); # typeidx,sym,loc
		    if (ka[1] == i)
			printf "    %-6d %s:%s\n", locs[k], ka[3], ka[2] | cmd
		}
	    } else {
		for (k in vcount) {
		    split(k,ka,SUBSEP);
		    if (ka[1] == i)
			printf "    %-6d %s\n", vcount[k], ka[2] | cmd
		}
	    }
	    close(cmd);
	}
    }
}
