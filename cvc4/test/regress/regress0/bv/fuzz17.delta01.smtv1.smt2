(set-option :incremental false)
(set-info :status sat)
(set-logic QF_BV)
(declare-fun v13 () (_ BitVec 16))
(declare-fun v9 () (_ BitVec 14))
(declare-fun v11 () (_ BitVec 13))
(declare-fun v3 () (_ BitVec 11))
(declare-fun v8 () (_ BitVec 9))
(declare-fun v4 () (_ BitVec 14))
(check-sat-assuming ( (let ((_let_0 (bvshl (_ bv1 16) ((_ sign_extend 2) v4)))) (let ((_let_1 ((_ extract 5 2) v8))) (let ((_let_2 (bvxnor ((_ sign_extend 9) _let_1) v11))) (let ((_let_3 (bvand ((_ sign_extend 14) (ite (bvugt (ite (bvugt ((_ sign_extend 1) v11) v4) (_ bv1 1) (_ bv0 1)) (ite (bvuge ((_ zero_extend 1) (_ bv21 8)) v8) (_ bv1 1) (_ bv0 1))) (_ bv1 1) (_ bv0 1))) ((_ sign_extend 2) v11)))) (let ((_let_4 ((_ zero_extend 7) v8))) (let ((_let_5 (bvand _let_4 v13))) (and (not (= (_ bv0 1) (ite (distinct (_ bv1 16) (bvsub (bvneg (bvnand _let_0 ((_ sign_extend 3) _let_2))) ((_ zero_extend 7) (bvashr ((_ zero_extend 8) (ite (bvugt _let_0 (_ bv0 16)) (_ bv1 1) (_ bv0 1))) ((_ extract 13 5) v9))))) (_ bv1 1) (_ bv0 1)))) (bvsle _let_1 ((_ sign_extend 3) (ite (bvslt _let_3 (_ bv0 15)) (_ bv1 1) (_ bv0 1)))) (or false (bvult (_ bv0 14) ((_ zero_extend 13) (bvcomp (ite (bvslt v4 (_ bv0 14)) (_ bv1 1) (_ bv0 1)) (ite (distinct (_ bv0 16) ((_ zero_extend 1) (bvshl _let_3 ((_ zero_extend 6) (bvsub ((_ zero_extend 8) (ite (= (bvsub (_ bv1 16) _let_5) ((_ sign_extend 7) v8)) (_ bv1 1) (_ bv0 1))) (bvnor (_ bv1 9) (_ bv1 9))))))) (_ bv1 1) (_ bv0 1))))) (= (bvadd ((_ sign_extend 8) (ite (bvsgt _let_5 _let_4) (_ bv1 1) (_ bv0 1))) ((_ sign_extend 8) (_ bv1 1))) (_ bv0 9))) (= (_ bv0 1) (ite (bvsle ((_ zero_extend 2) v9) _let_5) (_ bv1 1) (_ bv0 1))) (bvsge ((_ zero_extend 15) (ite (bvsge ((_ sign_extend 10) (ite (= _let_2 (_ bv1 13)) (_ bv1 1) (_ bv0 1))) v3) (_ bv1 1) (_ bv0 1))) (_ bv0 16)) (not (distinct (_ bv1 16) (bvxnor ((_ zero_extend 3) (bvxnor v11 (_ bv1 13))) (ite (= (_ bv1 1) (ite (bvsgt (_ bv1 14) v9) (_ bv1 1) (_ bv0 1))) v13 (_ bv0 16)))))))))))) ))
