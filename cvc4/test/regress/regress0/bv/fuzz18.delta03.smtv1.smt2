(set-option :incremental false)
(set-info :status sat)
(set-logic QF_BV)
(declare-fun v4 () (_ BitVec 4))
(declare-fun v2 () (_ BitVec 4))
(declare-fun v6 () (_ BitVec 4))
(check-sat-assuming ( (let ((_let_0 (bvlshr v2 v4))) (and (bvult ((_ sign_extend 3) (ite (bvugt ((_ zero_extend 3) (ite (bvuge v6 ((_ sign_extend 3) (_ bv1 1))) (_ bv1 1) (_ bv0 1))) (_ bv1 4)) (_ bv1 1) (_ bv0 1))) (bvashr _let_0 v6)) (distinct v4 (_ bv0 4)) (bvsle (_ bv0 1) (ite (bvslt (_ bv0 4) ((_ sign_extend 3) (ite (bvugt (_ bv1 4) ((_ zero_extend 3) (ite (bvslt (_ bv0 4) _let_0) (_ bv1 1) (_ bv0 1)))) (_ bv1 1) (_ bv0 1)))) (_ bv1 1) (_ bv0 1))))) ))
