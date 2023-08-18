#include "operation.h"

void print_edge(rexdd_edge_t ans)
{
    printf("ans's rule is %d\n", ans.label.rule);
    printf("ans's comp is %d\n", ans.label.complemented);
    printf("ans's swap is %d\n", ans.label.swapped);
    printf("ans's target is %s%llu\n", rexdd_is_terminal(ans.target)?"T":"", rexdd_is_terminal(ans.target)? rexdd_terminal_value(ans.target): ans.target);
}

char rexdd_edge_pattern(rexdd_edge_t* e)
{
    char type;
    switch (e->label.rule) {
        case rexdd_rule_EL0:
        case rexdd_rule_EL1:
        case rexdd_rule_AH0:
        case rexdd_rule_AH1:
            type = 'L';
            break;
        case rexdd_rule_EH0:
        case rexdd_rule_EH1:
        case rexdd_rule_AL0:
        case rexdd_rule_AL1:
            type = 'H';
            break;
        default:
            type = 'U';
    }
    return type;
}

rexdd_edge_t rexdd_expand_edgeXY(rexdd_edge_t* e, char xy)
{
    rexdd_edge_t ans;
    ans.label.rule = rexdd_rule_X;
    if (e->label.rule == rexdd_rule_EL0 || e->label.rule == rexdd_rule_EL1
        || e->label.rule == rexdd_rule_AL0 || e->label.rule == rexdd_rule_AL1) {
        ans.label.complemented = ((xy == 'x') || (xy == 'X')) ?
                                    ((e->label.rule == rexdd_rule_EL1) || (e->label.rule == rexdd_rule_AL1))
                                    : e->label.complemented;
        ans.label.swapped = ((xy == 'x') || (xy == 'X')) ? 0 : e->label.swapped;
        ans.target = ((xy == 'x') || (xy == 'X')) ? rexdd_make_terminal(0) : e->target;
    } else if (e->label.rule == rexdd_rule_EH0 || e->label.rule == rexdd_rule_EH1
        || e->label.rule == rexdd_rule_AH0 || e->label.rule == rexdd_rule_AH1) {
        ans.label.complemented = ((xy == 'y') || (xy == 'Y')) ?
                                    ((e->label.rule == rexdd_rule_EH1) || (e->label.rule == rexdd_rule_AH1))
                                    : e->label.complemented;
        ans.label.swapped = ((xy == 'y') || (xy == 'Y')) ? 0 : e->label.swapped;
        ans.target = ((xy == 'y') || (xy == 'Y')) ? rexdd_make_terminal(0) : e->target;
    } else {
        ans.label.complemented = e->label.complemented;
        ans.label.swapped = e->label.swapped;
        ans.target = e->target;
    }
    return ans;
}

rexdd_edge_t rexdd_build_L(rexdd_forest_t* F, rexdd_edge_t* ex, rexdd_edge_t* ey, uint32_t n, uint32_t m)
{
    rexdd_edge_t ans;

    /* Base cases that can directly return a long edge */
    if (ex->label.rule==rexdd_rule_X) {
        if (rexdd_edges_are_equal(ex, ey)) return *ex;
        if (rexdd_is_terminal(ex->target)) {
            rexdd_edge_label_t l;
            l.complemented = 0;
            l.swapped = 0;
            l.rule = (ex->label.complemented ^ rexdd_terminal_value(ex->target)) ? rexdd_rule_EL1 : rexdd_rule_EL0;
            rexdd_merge_edge(F, n, m-1, l, ey, &ans);
            return ans;
        }
        if (rexdd_is_terminal(ey->target) && (ey->label.rule == rexdd_rule_X)) {
            rexdd_edge_label_t l;
            l.complemented = 0;
            l.swapped = 0;
            l.rule = (ey->label.complemented ^ rexdd_terminal_value(ey->target)) ? rexdd_rule_AH1: rexdd_rule_AH0;
            rexdd_merge_edge(F, n, m-1, l, ex, &ans);
            return ans;
        }
    }
    
    /* Now we need to build this pattern */
    rexdd_edge_t re_high;
    rexdd_unpacked_node_t tmp;
    rexdd_edge_label_t l;
    l.complemented = 0;
    l.swapped = 0;
    l.rule = rexdd_rule_X;
    re_high = *ey;
    for (uint32_t i=m; i<=n; i++) {
        tmp.level = i;
        tmp.edge[0] = *ex;
        tmp.edge[1] = re_high;
        rexdd_reduce_edge(F, i, l, tmp, &re_high);
    }
    return re_high;
}

rexdd_edge_t rexdd_build_H(rexdd_forest_t* F, rexdd_edge_t* ex, rexdd_edge_t* ey, uint32_t n, uint32_t m)
{
    rexdd_edge_t ans;

    /* Base cases that can directly return a long edge */
    if (ey->label.rule==rexdd_rule_X) {
        if (rexdd_edges_are_equal(ex, ey)) return *ex;
        if (rexdd_is_terminal(ey->target)) {
            rexdd_edge_label_t l;
            l.complemented = 0;
            l.swapped = 0;
            l.rule = (ey->label.complemented ^ rexdd_terminal_value(ey->target)) ? rexdd_rule_EH1 : rexdd_rule_EH0;
            rexdd_merge_edge(F, n, m-1, l, ex, &ans);
            return ans;
        }
        if (rexdd_is_terminal(ex->target) && (ex->label.rule == rexdd_rule_X)) {
            rexdd_edge_label_t l;
            l.complemented = 0;
            l.swapped = 0;
            l.rule = (ex->label.complemented ^ rexdd_terminal_value(ex->target)) ? rexdd_rule_AL1: rexdd_rule_AL0;
            rexdd_merge_edge(F, n, m-1, l, ey, &ans);
            return ans;
        }
    }

    /* Now we need to build this pattern */
    rexdd_edge_t re_low;
    rexdd_unpacked_node_t tmp;
    rexdd_edge_label_t l;
    l.complemented = 0;
    l.swapped = 0;
    l.rule = rexdd_rule_X;
    re_low = *ex;
    for (uint32_t i=m; i<=n; i++) {
        tmp.level = i;
        tmp.edge[1] = *ey;
        tmp.edge[0] = re_low;
        rexdd_reduce_edge(F, i, l, tmp, &re_low);
    }
    return re_low;
}

rexdd_edge_t rexdd_build_U(rexdd_forest_t* F, rexdd_edge_t* ex, rexdd_edge_t* ey, rexdd_edge_t* ez, uint32_t n, uint32_t m)
{
    rexdd_edge_t ans;
    assert(n>=m);
    assert(m>(rexdd_is_terminal(ex->target)?0:rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, ex->target))));
    assert(m>(rexdd_is_terminal(ey->target)?0:rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, ey->target))));
    assert(m>(rexdd_is_terminal(ez->target)?0:rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, ez->target))));

    rexdd_edge_label_t l;
    l.complemented = 0;
    l.swapped = 0;
    l.rule = rexdd_rule_X;
    rexdd_unpacked_node_t tmp;
    tmp.level = n;
    tmp.edge[0] = rexdd_build_H(F, ex, ey, n-1, m);
    tmp.edge[1] = rexdd_build_L(F, ey, ez, n-1, m);
    rexdd_reduce_edge(F, n, l, tmp, &ans);
    return ans;
}

rexdd_edge_t rexdd_AND_LL(rexdd_forest_t* F, rexdd_edge_t* e1, rexdd_edge_t* e2, uint32_t n)
{
    /* this may be passing as parameters? */
    uint32_t m1, m2;
    if (rexdd_is_terminal(e1->target)) {
        m1 = 0;
    } else {
        m1 = rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, e1->target));
    }
    if (rexdd_is_terminal(e2->target)) {
        m2 = 0;
    } else {
        m2 = rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, e2->target));
    }

    assert(n>0);
    rexdd_edge_t x1,x2,y1,y2;
    x1 = rexdd_expand_edgeXY(e1,'x');
    x2 = rexdd_expand_edgeXY(e2,'x');
    y1 = rexdd_expand_edgeXY(e1,'y');
    if (m1 == m2) {
        y2 = rexdd_expand_edgeXY(e2,'y');
    } else {
        assert(m1>m2);
        y2 = rexdd_expand_childEdge(F, m1+1, e2, 1);
    }

    rexdd_edge_t x_and, y_and;
    x_and = rexdd_AND_edges(F, &x1, &x2, m1);
    y_and = rexdd_AND_edges(F, &y1, &y2, m1);
    return rexdd_build_L(F, &x_and, &y_and, n, m1+1);
}

rexdd_edge_t rexdd_AND_HH(rexdd_forest_t* F, rexdd_edge_t* e1, rexdd_edge_t* e2, uint32_t n)
{
    uint32_t m1, m2;
    if (rexdd_is_terminal(e1->target)) {
        m1 = 0;
    } else {
        m1 = rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, e1->target));
    }
    if (rexdd_is_terminal(e2->target)) {
        m2 = 0;
    } else {
        m2 = rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, e2->target));
    }

    rexdd_edge_t x1,x2,y1,y2;
    x1 = rexdd_expand_edgeXY(e1,'x');
    y1 = rexdd_expand_edgeXY(e1,'y');
    y2 = rexdd_expand_edgeXY(e2,'y');
    if (m1 == m2) {
        x2 = rexdd_expand_edgeXY(e2,'x');
    } else {
        assert(m1>m2);
        x2 = rexdd_expand_childEdge(F, m1+1, e2, 0);
    }

    rexdd_edge_t x_and, y_and;
    x_and = rexdd_AND_edges(F, &x1, &x2, m1);
    y_and = rexdd_AND_edges(F, &y1, &y2, m1);

    return rexdd_build_H(F, &x_and, &y_and, n, m1+1);
}

rexdd_edge_t rexdd_AND_LH(rexdd_forest_t* F, rexdd_edge_t* e1, rexdd_edge_t* e2, uint32_t n)
{
    uint32_t m1, m2, m;
    if (rexdd_is_terminal(e1->target)) {
        m1 = 0;
    } else {
        m1 = rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, e1->target));
    }
    if (rexdd_is_terminal(e2->target)) {
        m2 = 0;
    } else {
        m2 = rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, e2->target));
    }

    rexdd_edge_t x1,x2,y1,y2;
    x1 = rexdd_expand_edgeXY(e1,'x');
    y2 = rexdd_expand_edgeXY(e2,'y');
    if (m1 == m2) {
        y1 = rexdd_expand_edgeXY(e1,'y');
        x2 = rexdd_expand_edgeXY(e2,'x');
        m = m1;
    } else if (m1>m2){
        y1 = rexdd_expand_edgeXY(e1,'y');
        x2 = rexdd_expand_childEdge(F, m1+1, e2, 0);
        m = m1;
    } else {
        y1 = rexdd_expand_childEdge(F, m2+1, e1, 1);
        x2 = rexdd_expand_edgeXY(e2,'x');
        m = m2;
    }

    rexdd_edge_t x_and, y_and, z_and;
    x_and = rexdd_AND_edges(F, &x1, &x2, m);
    z_and = rexdd_AND_edges(F, &y1, &y2, m);
    if (n-m==1) {
        rexdd_edge_label_t l;
        l.rule = rexdd_rule_X;
        l.complemented = 0;
        l.swapped = 0;
        rexdd_unpacked_node_t tmp;
        tmp.level = n;
        tmp.edge[0] = x_and;
        tmp.edge[1] = z_and;
        rexdd_edge_t ans;
        rexdd_reduce_edge(F, n, l, tmp, &ans);
        return ans;
    }
    y_and = rexdd_AND_edges(F, &x1, &y2, m);

    return rexdd_build_U(F, &x_and, &y_and, &z_and, n, m+1);
}

rexdd_edge_t rexdd_AND_edges(rexdd_forest_t* F, const rexdd_edge_t* e1, const rexdd_edge_t* e2, uint32_t lvl)
{

    rexdd_edge_t edge1, edge2, edgeA;                         // the answer
    edge1 = *e1;
    edge2 = *e2;
    // normalize if they are constant edges
    rexdd_normalize_edge(&edge1);
    rexdd_normalize_edge(&edge2);

    // Base case 1: two edges are the same
    if (rexdd_edges_are_equal(&edge1, &edge2)) return edge1;

    // Base case 2: two edges are complemented
    if (rexdd_edges_are_complement(&edge1, &edge2)) {
        rexdd_set_edge(&edgeA,
                        rexdd_rule_X,
                        0,
                        0,
                        rexdd_make_terminal(0));
        return edgeA;
    }

    uint32_t m1, m2;
    if (rexdd_is_terminal(edge1.target)) {
        m1 = 0;
    } else {
        m1 = rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, edge1.target));
    }
    if (rexdd_is_terminal(edge2.target)) {
        m2 = 0;
    } else {
        m2 = rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, edge2.target));
    }

    // ordering
    if (m1 < m2) {
        //swap edge1 and edge2
        rexdd_edge_t tmp_edge;
        tmp_edge = edge1;
        edge1 = edge2;
        edge2 = tmp_edge;
        uint32_t tmp_m;
        tmp_m = m1;
        m1 = m2;
        m2 = tmp_m;
    }

    /*  Check Computing table HERE */
    if (rexdd_check_CT(F->CT, lvl, &edge1, &edge2, &edgeA)) return edgeA;

    // Case that edge1 is a short edge
    if (m1 == lvl) {
        rexdd_unpacked_node_t tmp_node;
        tmp_node.level = lvl;
        rexdd_edge_t x1, y1, x2, y2;
        x1 = rexdd_expand_childEdge(F,lvl, &edge1, 0);
        y1 = rexdd_expand_childEdge(F,lvl, &edge1, 1);
        x2 = rexdd_expand_childEdge(F,lvl, &edge2, 0);
        y2 = rexdd_expand_childEdge(F,lvl, &edge2, 1);

        tmp_node.edge[0] = rexdd_AND_edges(F, &x1, &x2, lvl-1);
        tmp_node.edge[1] = rexdd_AND_edges(F, &y1, &y2, lvl-1);
        rexdd_edge_label_t l;
        l.rule = rexdd_rule_X;
        l.complemented = 0;
        l.swapped = 0;
        rexdd_reduce_edge(F, lvl, l, tmp_node, &edgeA);
        
        /* Cache [n, edge1, edge2: edgeA] HERE */
        rexdd_cache_CT(F->CT, lvl, &edge1, &edge2, &edgeA);

        return edgeA;
    }

    // Here we have m1 >= m2, it's time to decide pattern types and use pattern AND
    char t1, t2;
    t1 = rexdd_edge_pattern(&edge1);
    t2 = rexdd_edge_pattern(&edge2);
    if (t1 == 'L') {
        if (t2 == 'L' || t2 == 'U') {
            edgeA = rexdd_AND_LL(F, &edge1, &edge2, lvl);
        } else {
            edgeA = rexdd_AND_LH(F, &edge1, &edge2, lvl);
        }
    } else if (t1 == 'H') {
        if (t2 == 'H' || t2 == 'U') {
            edgeA = rexdd_AND_HH(F, &edge1, &edge2, lvl);
        } else {
            edgeA = rexdd_AND_LH(F, &edge2, &edge1, lvl);
        }
    } else {
        if (t2 == 'L' || t2 == 'U') {
            edgeA = rexdd_AND_LL(F, &edge1, &edge2, lvl);
        } else {
            edgeA = rexdd_AND_HH(F, &edge1, &edge2, lvl);
        }
    }

    /* Cache [edge1, edge2: edgeA] HERE */
    rexdd_cache_CT(F->CT, lvl, &edge1, &edge2, &edgeA);
    return edgeA;
}

rexdd_edge_t rexdd_OR_edges(rexdd_forest_t* F, const rexdd_edge_t* e1, const rexdd_edge_t* e2, uint32_t lvl)
{
    // A or B = not ((not A) and (not B))
    rexdd_edge_t edge1, edge2, edgeA;                         // the answer
    edge1 = *e1;
    edge2 = *e2;
    rexdd_edge_com(&edge1);
    rexdd_edge_com(&edge2);
    edgeA = rexdd_AND_edges(F, &edge1, &edge2, lvl);
    rexdd_edge_com(&edgeA);
    return edgeA;
}

rexdd_edge_t rexdd_XOR_edges(rexdd_forest_t* F, const rexdd_edge_t* e1, const rexdd_edge_t* e2, uint32_t lvl)
{
    // A xor B = (A or B) and not (A and B)
    rexdd_edge_t edge1, edge2, edge_or, edge_and, edgeA;                         // the answer
    edge1 = *e1;
    edge2 = *e2;
    rexdd_edge_com(&edge1);
    rexdd_edge_com(&edge2);
    edge_or = rexdd_OR_edges(F, &edge1, &edge2, lvl);
    edge_and = rexdd_AND_edges(F, &edge1, &edge2, lvl);
    rexdd_edge_com(&edge_and);
    edgeA = rexdd_AND_edges(F, &edge_or, &edge_and, lvl);
    return edgeA;
}

rexdd_edge_t rexdd_NOT_edge(const rexdd_edge_t* e)
{
    rexdd_edge_t edgeA;
    edgeA = *e;
    rexdd_edge_com(&edgeA);
    return edgeA;
}

rexdd_edge_t rexdd_NAND_edges(rexdd_forest_t* F, const rexdd_edge_t* e1, const rexdd_edge_t* e2, uint32_t lvl)
{
    rexdd_edge_t edge1, edge2, edgeA;
    edge1 = *e1;
    edge2 = *e2;
    edgeA = rexdd_AND_edges(F, &edge1, &edge2, lvl);
    rexdd_edge_com(&edgeA);
    return edgeA;
}

rexdd_edge_t rexdd_NOR_edges(rexdd_forest_t* F, const rexdd_edge_t* e1, const rexdd_edge_t* e2, uint32_t lvl)
{
    rexdd_edge_t edge1, edge2, edgeA;
    edge1 = *e1;
    edge2 = *e2;
    edgeA = rexdd_OR_edges(F, &edge1, &edge2, lvl);
    rexdd_edge_com(&edgeA);
    return edgeA;
}

rexdd_edge_t rexdd_IMPLIES_edges(rexdd_forest_t* F, const rexdd_edge_t* e1, const rexdd_edge_t* e2, uint32_t lvl)
{
    rexdd_edge_t edge1, edge2, edgeA;
    edge1 = *e1;
    edge2 = *e2;
    rexdd_edge_com(&edge2);
    edgeA = rexdd_AND_edges(F, &edge1, &edge2, lvl);
    return edgeA;
}

rexdd_edge_t rexdd_EQUALS_edges(rexdd_forest_t* F, const rexdd_edge_t* e1, const rexdd_edge_t* e2, uint32_t lvl)
{
    // Since XOR is calling two ANDs, it may not be the most efficient, another implementation TBD?
    rexdd_edge_t edge1, edge2, edgeA;
    edge1 = *e1;
    edge2 = *e2;
    edgeA = rexdd_XOR_edges(F, &edge1, &edge2, lvl);
    rexdd_edge_com(&edgeA);
    return edgeA;
}