; COMMAND-LINE: --nl-ext
; EXPECT: unsat
(set-logic QF_NIA)
(declare-const n Int)
(declare-const i1 Int)
(declare-const i2 Int)
(declare-const j1 Int)
(declare-const j2 Int)
(assert (>= n 0))
(assert (not (= i1 i2)))
(assert (<= 0 i1))
(assert (<= i1 j1))
(assert (< j1 n))
(assert (<= 0 i2))
(assert (<= i2 j2))
(assert (< j2 n))
(assert (or
  (= (+ (* i1 n) j1) (+ (* i2 n) j2))
  (= (+ (* i1 n) j1) (+ (* j2 n) i2))
  (= (+ (* j1 n) i1) (+ (* i2 n) j2))
  (= (+ (* j1 n) i1) (+ (* j2 n) i2))))
(check-sat)
