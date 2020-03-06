; COMMAND-LINE: --incremental
; EXPECT: sat
; EXPECT: sat
; EXPECT: sat
; EXPECT: sat
; EXPECT: sat
; EXPECT: sat
; EXPECT: unsat
; EXPECT: sat
(set-logic QF_LRA)
(declare-fun x0 () Real)
(declare-fun x1 () Real)
(declare-fun x2 () Real)
(declare-fun x3 () Real)
(declare-fun x4 () Real)
(assert (or (<= (+ (* 30 x0 ) (* 33 x1 ) (* 40 x1 ) (* 27 x3 ) (* 17 x3 ) (* (- 18) x0 ) (* (- 30) x0 ) ) (- 46)) (> (+ (* 48 x3 ) (* 16 x1 ) (* (- 20) x4 ) (* (- 22) x1 ) (* (- 11) x3 ) (* (- 27) x0 ) (* 8 x2 ) (* (- 17) x1 ) (* (- 21) x1 ) (* 19 x0 ) ) 8) (not (> (+ (* 24 x4 ) (* (- 7) x0 ) (* 11 x4 ) (* (- 17) x1 ) (* 3 x1 ) (* 36 x1 ) (* (- 16) x0 ) ) 16)) ))
(assert (or (> (+ (* (- 30) x1 ) (* (- 6) x3 ) (* 33 x4 ) (* 9 x4 ) (* (- 47) x0 ) ) 30) (not (<= (+ (* 44 x3 ) (* (- 34) x1 ) (* (- 5) x2 ) (* (- 20) x1 ) (* 6 x1 ) (* (- 1) x4 ) (* 40 x2 ) (* 11 x4 ) (* 48 x3 ) (* 23 x3 ) (* (- 16) x2 ) ) 6)) ))
(assert (or (not (> (+ (* (- 9) x3 ) (* (- 49) x4 ) (* (- 23) x2 ) (* (- 36) x2 ) (* (- 11) x2 ) (* 5 x1 ) (* 10 x2 ) (* (- 6) x1 ) (* 1 x1 ) (* (- 34) x0 ) ) (- 1))) (> (+ (* 48 x4 ) (* 20 x0 ) ) 47) (not (<= (+ (* 39 x1 ) (* (- 7) x3 ) (* (- 3) x4 ) (* 43 x4 ) (* (- 45) x1 ) ) (- 49))) ))
(assert (>= (+ (* 38 x0 ) (* 11 x0 ) (* (- 23) x3 ) (* 5 x0 ) (* 7 x1 ) (* 25 x0 ) (* (- 30) x0 ) (* (- 21) x2 ) (* (- 20) x0 ) ) (- 26)) )
(assert (or (> (+ (* (- 25) x1 ) (* (- 41) x4 ) (* 34 x3 ) (* 45 x3 ) (* (- 34) x2 ) (* (- 47) x2 ) ) (- 7)) (< (+ (* 33 x1 ) (* (- 7) x1 ) (* (- 50) x3 ) (* 15 x2 ) ) 32) (= (+ (* 23 x3 ) (* 24 x0 ) (* (- 16) x3 ) (* (- 17) x4 ) (* 12 x0 ) (* (- 7) x4 ) (* (- 12) x0 ) (* 24 x3 ) (* 6 x2 ) ) (- 3)) ))
(assert (< (+ (* (- 11) x1 ) (* 29 x2 ) (* 10 x3 ) (* 21 x3 ) (* (- 27) x3 ) (* (- 18) x2 ) (* 31 x4 ) (* 29 x2 ) ) 46) )
(assert (= (+ (* 38 x2 ) (* 2 x0 ) (* 21 x1 ) (* (- 20) x3 ) (* 46 x3 ) (* (- 20) x1 ) (* (- 41) x2 ) (* 20 x2 ) ) (- 18)) )
(assert (or (= (+ (* 27 x3 ) (* 9 x4 ) (* (- 42) x4 ) (* (- 38) x2 ) (* (- 8) x3 ) (* (- 37) x1 ) (* 14 x4 ) (* 44 x0 ) (* 5 x4 ) (* (- 35) x0 ) (* (- 32) x2 ) ) 26) (> (+ (* (- 16) x0 ) (* (- 35) x0 ) (* 3 x3 ) (* (- 28) x3 ) (* 19 x4 ) (* (- 49) x3 ) (* (- 34) x1 ) (* (- 16) x0 ) (* 39 x4 ) (* 16 x4 ) (* 43 x3 ) ) (- 29)) (not (<= (+ (* (- 13) x4 ) (* 34 x0 ) (* (- 5) x1 ) (* 38 x3 ) (* 9 x3 ) (* 8 x1 ) (* (- 45) x1 ) (* (- 34) x4 ) ) 8)) ))
(check-sat)
(push 1)
(assert (or (<= (+ (* (- 18) x3 ) (* 41 x1 ) (* 7 x1 ) (* (- 34) x2 ) (* (- 8) x3 ) (* (- 13) x3 ) (* 6 x0 ) (* (- 22) x1 ) (* 17 x4 ) ) 19) (< (+ (* (- 33) x3 ) (* 20 x1 ) (* (- 8) x1 ) (* 17 x4 ) (* 17 x0 ) (* 23 x2 ) (* (- 40) x1 ) (* (- 35) x2 ) (* (- 15) x3 ) (* (- 13) x2 ) (* 47 x2 ) ) 43) ))
(assert (or (not (< (+ (* (- 2) x3 ) (* 1 x2 ) (* 11 x0 ) (* (- 32) x3 ) (* (- 7) x3 ) (* (- 5) x3 ) ) (- 40))) (< (+ (* (- 50) x2 ) (* (- 20) x2 ) ) 37) ))
(assert (or (= (+ (* 19 x2 ) (* (- 10) x1 ) ) (- 34)) (<= (+ (* 7 x0 ) (* 46 x1 ) (* 7 x0 ) (* (- 37) x4 ) (* (- 1) x2 ) (* 23 x0 ) (* 9 x3 ) (* 10 x0 ) (* (- 37) x0 ) (* (- 41) x1 ) (* 0 x0 ) ) 24) ))
(check-sat)
(push 1)
(check-sat)
(pop 1)
(assert (= (+ (* (- 20) x1 ) (* 32 x3 ) (* (- 21) x2 ) (* (- 9) x2 ) (* 5 x1 ) (* 4 x4 ) (* 42 x4 ) (* 6 x4 ) (* 22 x2 ) (* 32 x3 ) (* 42 x3 ) ) 1) )
(assert (or (not (>= (+ (* (- 39) x0 ) (* 19 x4 ) (* (- 1) x3 ) ) (- 47))) (not (<= (+ (* (- 40) x4 ) (* (- 10) x2 ) (* 22 x4 ) (* (- 20) x4 ) ) 30)) ))
(assert (not (= (+ (* (- 23) x0 ) (* 33 x4 ) (* (- 43) x0 ) (* (- 48) x4 ) (* 8 x1 ) (* (- 34) x1 ) (* 24 x3 ) (* 37 x4 ) (* (- 27) x2 ) (* (- 16) x4 ) ) (- 35))) )
(assert (not (>= (+ (* (- 1) x3 ) (* 19 x4 ) ) 29)) )
(assert (or (not (> (+ (* (- 36) x3 ) (* (- 16) x0 ) (* 12 x3 ) (* (- 17) x2 ) (* 1 x3 ) ) 22)) (< (+ (* (- 8) x2 ) (* (- 40) x1 ) (* (- 17) x4 ) (* 37 x1 ) (* 41 x2 ) (* (- 37) x1 ) (* (- 46) x3 ) ) (- 33)) ))
(assert (<= (+ (* 27 x1 ) (* 18 x4 ) ) 12) )
(assert (or (not (> (+ (* (- 43) x0 ) (* 43 x0 ) (* 36 x2 ) (* 21 x1 ) (* 11 x1 ) (* 32 x4 ) ) 24)) (not (< (+ (* 33 x0 ) (* 29 x3 ) (* 39 x3 ) (* 17 x4 ) (* 21 x0 ) (* 32 x2 ) (* (- 38) x1 ) (* (- 37) x0 ) (* 23 x4 ) ) (- 43))) (not (<= (+ (* 35 x4 ) (* 23 x1 ) (* 23 x0 ) (* (- 39) x0 ) (* (- 13) x4 ) (* (- 10) x1 ) (* (- 33) x2 ) (* 28 x1 ) (* 41 x4 ) (* 43 x4 ) ) 23)) ))
(check-sat)
(pop 1)
(assert (or (not (= (+ (* (- 12) x0 ) (* (- 26) x2 ) (* (- 34) x1 ) (* 46 x0 ) (* (- 38) x4 ) (* (- 45) x4 ) ) 0)) (not (>= (+ (* (- 23) x2 ) (* 9 x2 ) (* 48 x0 ) (* (- 6) x2 ) (* (- 40) x1 ) (* (- 19) x0 ) (* (- 21) x4 ) ) 13)) (not (<= (+ (* 45 x1 ) (* 28 x3 ) (* (- 13) x1 ) ) 40)) ))
(check-sat)
(push 1)
(assert (or (>= (+ (* (- 31) x0 ) (* 39 x3 ) (* (- 43) x2 ) (* (- 12) x4 ) (* (- 46) x0 ) (* 46 x3 ) (* 19 x0 ) (* (- 8) x4 ) (* 41 x3 ) (* 34 x0 ) ) (- 10)) (not (>= (+ (* (- 20) x3 ) (* (- 19) x4 ) (* (- 33) x3 ) (* 18 x2 ) (* (- 47) x1 ) (* 28 x0 ) (* 6 x0 ) (* (- 23) x1 ) (* 6 x0 ) (* 0 x0 ) ) (- 25))) ))
(assert (or (not (>= (+ (* 19 x0 ) (* 27 x4 ) (* (- 45) x4 ) (* (- 27) x2 ) (* (- 5) x3 ) (* (- 20) x0 ) ) 16)) (not (> (+ (* 45 x4 ) (* (- 22) x4 ) (* 46 x4 ) (* (- 1) x1 ) (* 12 x3 ) (* (- 7) x0 ) (* 15 x3 ) (* 28 x4 ) (* 26 x4 ) (* 35 x2 ) (* (- 35) x1 ) ) 11)) ))
(assert (or (not (<= (+ (* 35 x2 ) (* 44 x3 ) (* 44 x2 ) ) (- 28))) (> (+ (* 39 x3 ) (* (- 6) x2 ) (* 2 x4 ) (* (- 5) x4 ) (* 45 x2 ) (* 40 x1 ) (* 4 x1 ) (* (- 8) x0 ) (* (- 33) x3 ) ) 45) ))
(assert (or (> (+ (* 46 x4 ) (* (- 4) x0 ) ) 5) (>= (+ (* (- 21) x1 ) (* 22 x0 ) (* 19 x3 ) (* (- 34) x3 ) (* 41 x1 ) (* (- 1) x1 ) (* (- 39) x1 ) ) 41) ))
(assert (<= (+ (* (- 38) x4 ) (* 43 x1 ) (* 46 x4 ) (* 14 x1 ) (* 49 x3 ) (* (- 18) x3 ) (* 38 x0 ) (* (- 36) x4 ) (* 24 x4 ) (* 28 x0 ) (* (- 14) x3 ) ) (- 23)) )
(assert (not (< (+ (* 36 x0 ) (* (- 19) x4 ) (* 5 x3 ) ) 26)) )
(assert (or (> (+ (* (- 46) x3 ) (* 1 x0 ) (* 37 x0 ) (* (- 44) x0 ) (* 45 x3 ) (* (- 19) x1 ) (* 14 x3 ) (* (- 16) x2 ) (* 35 x2 ) (* 47 x0 ) (* (- 21) x3 ) ) 30) (< (+ (* (- 11) x3 ) (* 7 x2 ) (* (- 5) x3 ) ) (- 37)) ))
(assert (not (> (+ (* 23 x4 ) (* (- 45) x0 ) ) 6)) )
(assert (or (not (> (+ (* 45 x4 ) (* (- 38) x2 ) (* (- 13) x4 ) (* 11 x0 ) (* (- 32) x0 ) (* 22 x2 ) ) (- 23))) (> (+ (* (- 32) x0 ) (* 24 x3 ) (* (- 26) x4 ) (* (- 6) x2 ) ) (- 20)) ))
(check-sat)
(push 1)
(assert (or (>= (+ (* 19 x4 ) (* 39 x3 ) (* 0 x2 ) (* (- 46) x2 ) (* (- 44) x4 ) (* (- 2) x4 ) (* 1 x4 ) (* 14 x1 ) (* 47 x4 ) (* 3 x3 ) (* (- 12) x1 ) ) 0) (<= (+ (* 4 x0 ) (* 17 x4 ) (* (- 26) x0 ) (* (- 30) x1 ) (* 45 x0 ) ) 20) ))
(assert (or (not (< (+ (* 24 x2 ) (* (- 17) x2 ) (* 3 x0 ) ) 32)) (not (< (+ (* (- 41) x4 ) (* 15 x4 ) (* 16 x4 ) ) (- 31))) (not (< (+ (* 24 x4 ) (* 1 x1 ) ) 19)) ))
(assert (or (< (+ (* 2 x1 ) (* (- 12) x0 ) (* (- 37) x2 ) (* 22 x4 ) (* (- 47) x4 ) ) (- 22)) (>= (+ (* 13 x0 ) (* (- 49) x1 ) (* 41 x3 ) (* 10 x4 ) (* (- 25) x0 ) (* 37 x1 ) (* 32 x3 ) ) 10) (= (+ (* (- 50) x4 ) (* 49 x2 ) (* (- 49) x3 ) (* 9 x1 ) (* 1 x1 ) (* (- 30) x4 ) (* (- 44) x0 ) ) 33) ))
(assert (or (< (+ (* (- 45) x1 ) (* 34 x3 ) (* (- 41) x4 ) (* 7 x3 ) (* (- 2) x1 ) (* 26 x4 ) (* (- 17) x1 ) (* (- 36) x2 ) (* 48 x2 ) (* (- 7) x1 ) (* 0 x4 ) ) (- 34)) (not (< (+ (* (- 34) x3 ) (* (- 22) x0 ) (* (- 17) x0 ) ) 35)) ))
(assert (or (not (> (+ (* 48 x2 ) (* 13 x2 ) ) (- 24))) (> (+ (* (- 15) x4 ) (* 32 x3 ) ) (- 19)) (not (= (+ (* (- 8) x0 ) (* (- 15) x3 ) (* (- 39) x3 ) (* 15 x0 ) (* (- 49) x1 ) (* 16 x1 ) ) 33)) ))
(assert (not (>= (+ (* 41 x2 ) (* 35 x4 ) (* 40 x4 ) (* 49 x3 ) ) 23)) )
(assert (or (not (>= (+ (* (- 45) x0 ) (* (- 40) x4 ) (* 0 x1 ) (* 15 x1 ) (* (- 38) x3 ) (* 36 x1 ) (* (- 12) x1 ) (* 47 x0 ) (* 47 x2 ) (* (- 34) x3 ) ) (- 16))) (not (>= (+ (* (- 18) x4 ) (* (- 10) x0 ) (* 20 x2 ) (* (- 8) x4 ) (* (- 25) x1 ) (* (- 6) x2 ) (* 30 x2 ) ) (- 41))) (> (+ (* 26 x3 ) (* (- 22) x1 ) ) 23) ))
(check-sat)
(pop 1)
(check-sat)
(push 1)
