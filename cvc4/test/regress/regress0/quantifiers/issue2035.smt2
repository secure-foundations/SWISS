; COMMAND-LINE: --inst-when=full --full-saturate-quant
; EXPECT: unsat
(set-logic AUFLIA)
(set-info :status unsat)
(declare-fun _substvar_37_ () Int)
(declare-fun _substvar_33_ () Int)
(declare-fun _substvar_32_ () Int)
(declare-sort A 0)
(declare-sort PZA 0)
(declare-fun MS (Int A PZA) Bool)
(declare-fun length (PZA Int) Bool)
(declare-fun p () PZA)
(assert (! (exists ((n55 Int)) (and true true (forall ((x862 Int) (x863 A) (x864 A)) (=> (and (MS x862 x863 p) (MS x862 x864 p)) (= x863 x864))) true)) :named hyp30))
(assert (! (exists ((x1298 A) (x1299 A) (x1300 Int)) (exists ((x1302 Int)) (length p 0))) :named hyp42))
(assert (! (not (exists ((n67 Int)) (and true true (forall ((x1308 Int) (x1309 A) (x1310 A)) (=> (and (exists ((i114 Int)) (and true true (= _substvar_32_ _substvar_33_) (exists ((x1312 Int)) (and (forall ((x1313 Int)) (=> (length p 0) (= x1312 (+ (- 0 _substvar_33_) 1)))) (MS x1312 x1309 p))))) (exists ((i115 Int)) (and true true (= _substvar_32_ _substvar_37_) (exists ((x1315 Int)) (and (forall ((x1316 Int)) (=> (length p 0) (= x1315 (+ (- 0 _substvar_37_) 1)))) (MS x1315 x1310 p)))))) (= x1309 x1310))) true))) :named goal))
(check-sat)
