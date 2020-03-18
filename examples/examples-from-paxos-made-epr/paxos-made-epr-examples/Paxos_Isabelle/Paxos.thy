section {* The Paxos I/O-Automaton *}
  
theory Paxos
  imports Main Simulations "~~/src/HOL/Eisbach/Eisbach_Tools"
begin

text "The theory Simulations is part of the I/O-Automata formalization found at @{url \<open>https://github.com/nano-o/IO-Automata\<close>}"

interpretation IOA .
  
datatype 'v paxos_action =
  Internal | Decision 'v
  
text {* We specify Paxos following the IVy FOL specification. 
Rounds are natural numbers, and We use the round @{text "0"} to represent the round @{text "\<bottom>"}.
We use the type variable @{typ "'n"} for nodes and @{typ "'v"} for values. *}
  
definition paxos_asig where
  "paxos_asig \<equiv> \<lparr> inputs = {}, outputs = {Decision v | v . True}, internals = {Internal} \<rparr>"

record ('v,'n) state =
  start_round_msg :: "nat set"
  join_ack_msg :: "('n \<times> nat \<times> nat \<times> 'v) set"
  propose_msg :: "(nat \<times> 'v) set"
  vote_msg :: "('n \<times> nat \<times> 'v) set"
  decision :: "(nat \<times> 'v) set"

locale paxos_ioa =
  fixes quorums::"'n set set"
    -- "We will make assumptions on quorums only in the correctness proof. "
begin

definition start where
  -- {* The initial state *}
  "start \<equiv> {\<lparr> start_round_msg = {}, join_ack_msg = {}, propose_msg = {},
    vote_msg = {}, decision = {} \<rparr>}"

definition start_round where
  "start_round r s s' \<equiv>
    r \<noteq> 0 \<and> s' = s\<lparr>start_round_msg := start_round_msg s \<union> {r}\<rparr>"

definition join_round where
  "join_round n r maxr maxv s s' \<equiv>
    r \<noteq> 0 \<and> r \<in> start_round_msg s \<and>
    (\<forall> r' \<ge> r . \<forall> r'' v . (n, r', r'', v) \<notin> join_ack_msg s) \<and>
    ((maxr = 0 \<and> (\<forall> r' v . r' < r \<longrightarrow> (n, r', v) \<notin> vote_msg s)) \<or>
        (maxr \<noteq> 0 \<and> maxr < r \<and> (n, maxr, maxv) \<in> vote_msg s
        \<and> (\<forall> r' v . r' < r \<and> (n, r', v) \<in> vote_msg s \<longrightarrow> r' \<le> maxr))) \<and>
    s' = s\<lparr>join_ack_msg := join_ack_msg s \<union> {(n, r, maxr, maxv)}\<rparr>"

definition propose where
  "propose r q v s s' \<equiv> 
    r \<noteq> 0 \<and> (\<forall> v . (r, v) \<notin> propose_msg s) \<and> q \<in> quorums \<and>
    (\<forall> n \<in> q . \<exists> maxr maxv . (n, r, maxr, maxv) \<in> join_ack_msg s) \<and>
    (\<exists> n maxr maxv . (n, r, maxr, maxv) \<in> join_ack_msg s \<and>
      (\<forall> n' r maxr' maxv' . n' \<in> q \<and> (n', r, maxr', maxv') \<in> join_ack_msg s \<longrightarrow> maxr' \<le> maxr) \<and>
      (maxr = 0 \<or> v = maxv) \<and>
      s' = s\<lparr>propose_msg := propose_msg s \<union> {(r, v)}\<rparr>)"

definition vote where
  "vote n r v s s' \<equiv> 
    r \<noteq> 0 \<and> (r, v) \<in> propose_msg s \<and> 
    (\<forall> r' maxr maxv . r' > r \<longrightarrow> (n, r', maxr, maxv) \<notin> join_ack_msg s) \<and>
    (\<exists> maxr maxv . (n,r,maxr,maxv) \<in> join_ack_msg s) \<and>
    s' = s\<lparr>vote_msg := vote_msg s \<union> {(n, r, v)}\<rparr>"
  
definition decide where
  "decide r v q s s' \<equiv>
    r \<noteq> 0 \<and> q \<in> quorums \<and> (\<forall> n \<in> q . (n, r, v) \<in> vote_msg s) \<and>
    s' = s\<lparr>decision := decision s \<union> {(r, v)}\<rparr>"

fun trans_rel :: "('v, 'n) state \<Rightarrow> 'v paxos_action \<Rightarrow> ('v, 'n) state \<Rightarrow> bool" where
  "trans_rel s Internal s' = (
    (\<exists> r . start_round r s s') \<or>
    (\<exists> n r maxr maxv . join_round n r maxr maxv s s') \<or>
    (\<exists> r q v . propose r q v s s') \<or>
    (\<exists> n r v . vote n r v s s') )" |
  "trans_rel s (Decision v) s' =  (\<exists> r q . decide r v q s s')"

definition trans where
  "trans \<equiv> { (s,a,s') . trans_rel s a s'}"

definition paxos_ioa where
  "paxos_ioa \<equiv> \<lparr>ioa.asig = paxos_asig, start = start, trans = trans\<rparr>"

lemmas simps = paxos_ioa_def paxos_asig_def start_def start_round_def trans_def propose_def join_round_def 
  vote_def decide_def

end

section {* The Abstract Paxos I/O-automaton *}

record ('v,'n) abstract_state =
  votes :: "('n \<times> nat \<times> 'v) set"
  joined_round :: "('n \<times> nat) set"
  proposal :: "(nat \<times> 'v) set"
  decision :: "(nat \<times> 'v) set"

locale abstract_paxos_ioa =
  fixes quorums::"'n set set"
begin

definition start where
  -- {* The initial state *}
  "start \<equiv> {\<lparr> votes = {}, joined_round = {}, proposal = {}, decision = {} \<rparr>}"

definition vote where
  "vote n r v s s' \<equiv>
    r \<noteq> 0 \<and> (r, v) \<in> proposal s \<and> (\<forall> r' > r . (n, r') \<notin> joined_round s) \<and>
    (n, r) \<in> joined_round s \<and> s' = s\<lparr>votes := (votes s) \<union> {(n, r, v)}\<rparr>"

definition join_round where
  "join_round n r s s' \<equiv>
    (\<forall> r' \<ge> r . (n, r') \<notin> joined_round s) \<and> s' = s\<lparr>joined_round := (joined_round s) \<union> {(n, r)}\<rparr>"

definition propose where 
  "propose r q v s s' \<equiv>
    r \<noteq> 0 \<and> q \<in> quorums \<and>
    (\<forall> v . (r, v) \<notin> proposal s) \<and>
    (\<forall> n \<in> q . (n, r) \<in> joined_round s) \<and>
    (\<exists> maxr maxn . maxr < r \<and>
      ((maxr = 0 \<and> (\<forall> n \<in> q . \<forall> r' < r . \<forall> v . (n, r', v) \<notin> votes s)) \<or> 
        (maxr \<noteq> 0 \<and> (maxn, maxr, v) \<in> votes s \<and> (\<forall> n \<in> q . \<forall> r' < r . \<forall> v .
          (n, r', v) \<in> votes s \<longrightarrow> r' \<le> maxr)))) \<and>
      s' = s\<lparr>proposal := (proposal s) \<union> {(r,v)}\<rparr>"

definition decide where  
  "decide r v s s' \<equiv> 
    r \<noteq> 0 \<and> (\<exists> q \<in> quorums . \<forall> n \<in> q . (n, r, v) \<in> votes s) \<and>
    s' = s\<lparr>decision := decision s \<union> {(r, v)}\<rparr>"

fun trans_rel :: "('v, 'n) abstract_state \<Rightarrow> 'v paxos_action \<Rightarrow> ('v, 'n) abstract_state \<Rightarrow> bool" where
  "trans_rel s Internal s' = (
    (\<exists> n r . join_round n r s s') \<or>
    (\<exists> r q v . propose r q v s s') \<or>
    (\<exists> n r v . vote n r v s s'))" |
  "trans_rel s (Decision v) s' =  (\<exists> r . decide r v s s')"
  
definition trans where
  "trans \<equiv> { (s,a,s') . trans_rel s a s'}"

definition abstract_paxos_ioa :: "(('v, 'n) abstract_state, 'v paxos_action) ioa" where
  "abstract_paxos_ioa \<equiv> \<lparr>ioa.asig = paxos_asig, start = start, trans = trans\<rparr>"

end

section {* Proof Automation setup *}

text {* A very simple verification-condition generator for proving invariants, that unfolds definitions and 
  inserts proved invariants as premises. *}
  
named_theorems invs named_theorems inv_defs named_theorems ioa_defs

method rm_reachable = (match premises in R[thin]:"reachable ?ioa ?s" \<Rightarrow> \<open>-\<close>)

lemma reach_and_inv_imp_p:"\<lbrakk>reachable ioa s; invariant ioa i\<rbrakk> \<Longrightarrow> i s"
  by (auto simp add:invariant_def)
    
method instantiate_invs declares invs =
  (match premises in I[thin]:"invariant ?ioa ?inv" and R:"reachable ?ioa ?s" \<Rightarrow> \<open>
    print_fact I, insert reach_and_inv_imp_p[OF R I]\<close>)+
    
method inv_vcg declares invs ioa_defs inv_defs = (
  rule invariantI,
  force simp add:ioa_defs inv_defs, 
  (insert invs, instantiate_invs)?, rm_reachable, simp add:ioa_defs)

locale paxos_proof = abs:abstract_paxos_ioa quorums + paxos_ioa quorums 
    for quorums :: "'n set set" +
    fixes abs_ioa  :: "(('v, 'n) abstract_state, 'v paxos_action) ioa"
      and con_ioa :: "(('v, 'n) state, 'v paxos_action) ioa"
    defines "abs_ioa \<equiv> abs.abstract_paxos_ioa" and "con_ioa \<equiv> paxos_ioa"  
    assumes "\<And> q1 q2 . \<lbrakk>q1 \<in> quorums; q2 \<in> quorums\<rbrakk> \<Longrightarrow> q1 \<inter> q2 \<noteq> {}"
    and "quorums \<noteq> {}" and "\<And> q . q \<in> quorums \<Longrightarrow> finite q"
    and "finite quorums"
begin
  
lemma quorum_inter_witness[elim]:
  assumes "q1 \<in> quorums" and "q2 \<in> quorums"
  obtains a where "a \<in> q1" and "a \<in> q2"
  using assms paxos_proof_axioms
  by (metis disjoint_iff_not_equal paxos_proof_def)
  
lemmas asig_simps =  externals_def abs.abstract_paxos_ioa_def paxos_asig_def con_ioa_def abs_ioa_def paxos_ioa_def
lemmas trans_simps = con_ioa_def paxos_ioa_def start_def abs_ioa_def abs.abstract_paxos_ioa_def abs.start_def
  is_trans_def trans_def abs.trans_def
lemmas act_defs =  abs.propose_def abs.join_round_def abs.vote_def abs.decide_def
  vote_def start_round_def join_round_def decide_def propose_def
  
declare trans_simps[ioa_defs]
  
section {* Proof of Abstract Paxos *}
  
definition safety where safety_def[inv_defs]:
  "safety s \<equiv> \<forall> v v' r r' . (r,v) \<in> decision s \<and> (r',v') \<in> decision s \<longrightarrow> v = v'"

definition invariant_1 where invariant_1_def[inv_defs]:
  "invariant_1 s \<equiv> (\<forall> n r v . (n, r, v) \<in> votes s \<longrightarrow> (r, v) \<in> proposal s)
    \<and> (\<forall> r v v' . (r, v) \<in> proposal s \<and> (r, v') \<in> proposal s \<longrightarrow> v = v')"

definition choosable where 
  "choosable s r v \<equiv> \<exists> q \<in> quorums . \<forall> n \<in> q . 
    (\<exists> r' > r . (n, r') \<in> joined_round s) \<longrightarrow> (n, r, v) \<in> votes s"
  
definition invariant_2 where invariant_2_def[inv_defs]:
  "invariant_2 s \<equiv> \<forall> r r' v v' . r' < r \<and> (r, v) \<in> proposal s \<and> 
    choosable s r' v' \<longrightarrow> v = v'"
  
definition invariant_3 where invariant_3_def[inv_defs]:
  "invariant_3 s \<equiv> \<forall> v r . (r,v) \<in> decision s \<longrightarrow> (\<exists> q \<in> quorums . \<forall> n \<in> q . (n, r, v) \<in> votes s)"
  
declare act_defs[simp]

lemma abs_trans_cases: 
  -- "A case split rule for analyzing the transition relation by cases"
  assumes "abs.trans_rel s a s'"
  obtains
  (abs_join_round) n r where "abs.join_round n r s s'"  and "a = Internal" |
  (abs_propose) r q v where "abs.propose r q v s s'"  and "a = Internal"|
  (abs_vote) n r v where "abs.vote n r v s s'"  and "a = Internal"|
  (abs_decide) r v where "abs.decide r v s s'" and "a = Decision v"
  by (meson abstract_paxos_ioa.trans_rel.elims(2) assms)
  
lemma invariant_1:
  "invariant abs_ioa invariant_1"
  apply (inv_vcg, elim abs_trans_cases)
  apply (auto simp add:inv_defs)
  done
declare invariant_1[invs]
   
lemma invariant_2:
  "invariant abs_ioa invariant_2"
  apply (inv_vcg)
  subgoal premises prems for s s' a using prems(2)
  proof (cases rule:abs_trans_cases)
    case (abs_join_round n r)
    have "votes s' = votes s" 
      and "\<And> n r . (n, r) \<in> joined_round s \<Longrightarrow> (n,r) \<in> joined_round s'" 
      and "proposal s' = proposal s" using abs_join_round by simp_all
    thus ?thesis using prems(1) apply (auto simp add:invariant_2_def choosable_def)
      by meson
  next
    case (abs_propose r q v)
    have "votes s' = votes s" and "joined_round s' = joined_round s" 
      and "proposal s' = (proposal s) \<union> {(r,v)}"
      using abs_propose by auto
    moreover
    have "v' = v" if "r' < r" and "choosable s r' v'" for v' r'
    proof -
      have q_joined:"\<forall> n \<in> q . (n,r) \<in> joined_round s" and "q \<in> quorums" using abs_propose 
        by simp_all
      obtain rmax n where "q \<in> quorums" and "rmax < r"
        "(rmax = 0 \<and> (\<forall> n \<in> q . \<forall> r' < r . \<forall> v . (n, r', v) \<notin> votes s))
         \<or> (rmax \<noteq> 0 \<and> (n, rmax, v) \<in> votes s  
            \<and> (\<forall> n' \<in> q . \<forall> r' < r . \<forall> v . (n', r', v) \<in> votes s \<longrightarrow> r' \<le> rmax))"
        using abs_propose by auto
      with this consider
        (no_votes) "rmax = 0" and "\<forall> n \<in> q . \<forall> r' < r . \<forall> v . (n, r', v) \<notin> votes s" |
        (votes) "rmax \<noteq> 0" and "(n, rmax, v) \<in> votes s" 
          and "\<forall> n' \<in> q . \<forall> r' < r .  \<forall> v . (n', r', v) \<in> votes s \<longrightarrow> r' \<le> rmax" by auto
      thus ?thesis 
      proof (cases)
        case no_votes
        hence False using q_joined that \<open>q \<in> quorums\<close> apply (auto simp add:choosable_def)
          by (meson quorum_inter_witness)
        thus ?thesis by auto
      next
        case votes
        from `choosable s r' v'` obtain n' where "(n', r', v') \<in> votes s"
          using \<open>q \<in> quorums\<close> \<open>r' < r\<close> q_joined by (force simp add:choosable_def)
        consider (a) "r' < rmax" | (b) "r' = rmax" | (c) "r' > rmax"
          using nat_neq_iff by blast
        then show ?thesis 
        proof (cases)
          case a
          with prems(1,3) votes(2) that(2) `rmax < r` show ?thesis
            by (auto simp add:invariant_2_def invariant_1_def, blast)
        next
          case b
          from `choosable s r' v'` obtain n' where "( n', r', v') \<in> votes s"
            using \<open>q \<in> quorums\<close> \<open>r' < r\<close> q_joined by (force simp add:choosable_def)
          with prems(3) votes(2) b show ?thesis by (force simp add:invariant_1_def split:option.splits)
        next
          case c
          hence "\<forall> v . (n, r', v) \<notin> votes s" if "n \<in> q" for n
            using votes(3) that `r' < r` by force
          with \<open>choosable s r' v'\<close> \<open>q \<in> quorums\<close> q_joined `r' < r` have False by (force simp add:choosable_def)
          thus ?thesis by auto
        qed
      qed
    qed
    ultimately show ?thesis using prems(1) apply (auto simp add:invariant_2_def choosable_def)
      by (blast, meson)
  next
    case (abs_vote n r v)
    have "joined_round s' = joined_round s" and "proposal s' = proposal s"
      and "\<And> n r v . (n, r, v) \<in> votes s' \<and> (n, r, v) \<notin> votes s \<longrightarrow> 
        ((n, r) \<in> joined_round s \<and> (\<forall> r' > r . (n, r') \<notin> joined_round s))" using abs_vote by auto
    thus ?thesis using prems(1) prems(2) apply (auto simp add:invariant_2_def invariant_1_def choosable_def)
        by meson
  next
    case (abs_decide r v)
    then show ?thesis using prems(1) by (auto simp add:invariant_2_def choosable_def)
  qed
  done
declare invariant_2[invs]
  
lemma invariant_3:
  "invariant abs_ioa invariant_3"
  apply (inv_vcg, elim abs_trans_cases)
     apply (simp_all add:inv_defs) 
  apply metis
  done
declare invariant_3[invs]
  
theorem safety:
  "invariant abs_ioa safety"
proof -
  have "safety s" if "invariant_3 s" and "invariant_2 s" and "invariant_1 s" for s using that
    apply (auto simp add:inv_defs choosable_def)
    by (metis not_less_iff_gr_or_eq quorum_inter_witness)
  thus ?thesis
    using IOA.invariant_def invariant_1 invariant_2 invariant_3 by blast
qed

declare invariant_1[invs del] invariant_2[invs del] invariant_3[invs del]
  
section {* Proof of Paxos by refinement to abstract paxos. *}
  
definition invariant_4 where invariant_4_def[inv_defs]:
  "invariant_4 s \<equiv> \<forall> n r maxr maxv . (n, r, maxr, maxv) \<in> join_ack_msg s \<and> maxr \<noteq> 0
    \<longrightarrow> (maxr < r \<and> (n, maxr, maxv) \<in> vote_msg s \<and> (\<forall> r' < r . \<forall> v . (n, r', v) \<in> vote_msg s \<longrightarrow> r' \<le> maxr))"
  
definition invariant_5 where invariant_5_def[inv_defs]:
  "invariant_5 s \<equiv> \<forall> n r maxr maxv . (n, r, maxr, maxv) \<in> join_ack_msg s \<and> maxr = 0
    \<longrightarrow> (\<forall> r' v . r' < r \<longrightarrow> (n, r', v) \<notin> vote_msg s)"
  
lemma trans_cases: 
  -- "A case split rule for analyzing the transition relation by cases"
  assumes "trans_rel s a s'"
  obtains
  (start_round) r where "start_round r s s'" and "a = Internal" |
  (join_round) n r maxr maxv where "join_round n r maxr maxv s s'" and "a = Internal" |
  (propose) r q v where "propose r q v s s'"  and "a = Internal"|
  (vote) n r v where "vote n r v s s'"  and "a = Internal"|
  (decide) r v q where "decide r v q s s'" and "a = Decision v"
  by (meson assms trans_rel.elims(2))
  
lemma invariant_4:
  "invariant con_ioa invariant_4"
  apply (inv_vcg, elim trans_cases)
      apply (force simp add:inv_defs)
     apply (simp add:inv_defs) apply blast
    apply (simp add:inv_defs) apply force
   apply (simp add:inv_defs) apply blast 
  apply (simp add:inv_defs) 
  done
declare invariant_4[invs]
  
lemma invariant_5:
  "invariant con_ioa invariant_5"
  apply (inv_vcg, elim trans_cases)
      apply (force simp add:inv_defs) defer
     apply (simp add:inv_defs)
     apply (metis state.select_convs(2) state.select_convs(4) state.surjective state.update_convs(3))
    apply (force simp add:inv_defs)
   apply (force simp add:inv_defs)
  apply (simp add:inv_defs) apply blast
  done
declare invariant_5[invs]
  
declare act_defs[simp del]
  
definition ref_map where 
  "ref_map s \<equiv> \<lparr>votes = vote_msg s, joined_round = {(n,r) . \<exists> r' v . (n,r,r',v) \<in> join_ack_msg s},
    proposal = propose_msg s, decision = state.decision s\<rparr>"
  
lemmas ref_proof_simps = ref_map_def asig_simps trans_simps trace_def schedule_def filter_act_def
  
lemma refinement:
  shows "is_ref_map ref_map con_ioa abs_ioa"
  apply (auto simp add:is_ref_map_def)
   apply (simp add:ref_map_def ioa_defs)
  apply (insert invs)
  apply (instantiate_invs)
  apply (simp add:trans_simps refines_def trace_match_def)
  subgoal premises prems for s t a using prems(2)
  proof (induct rule:trans_cases)
    case (start_round r)
    then show ?case
      apply (intro exI[where x="[]"])
      apply (auto simp add:ref_proof_simps start_round_def)
      done
  next
    case (join_round n r maxr maxv)
    let ?s' = "ref_map s"
    let ?t' = "(ref_map s)\<lparr>joined_round := joined_round (ref_map s) \<union> {(n,r)}\<rparr>"
    have "abs.join_round n r ?s' ?t'" using join_round(1) 
      by (auto simp add:join_round_def abs.join_round_def ref_map_def)
    moreover 
    have "ref_map t = ?t'" using join_round(1) by (auto simp add:join_round_def ref_map_def)
    ultimately show ?case using join_round(2)
      apply (intro exI[where x="[(Internal, ?t')]"])
      apply (auto simp add:ref_proof_simps)
      done
  next
    case (propose r q v)
    let ?s' = "ref_map s"
    let ?t' = "(ref_map s)\<lparr>proposal := proposal (ref_map s) \<union> {(r,v)}\<rparr>"
    have "abs.propose r q v ?s' ?t'"
    proof -
      obtain n maxr maxv where con_pre:"r \<noteq> 0 \<and> (\<forall> v . (r, v) \<notin> propose_msg s) \<and> q \<in> quorums \<and>
          (\<forall> n' \<in> q . \<exists> maxr maxv . (n',r,maxr,maxv) \<in> join_ack_msg s) 
          \<and> (n, r, maxr, maxv) \<in> join_ack_msg s \<and>
          (\<forall> n' r maxr' maxv' . n' \<in> q \<and> (n', r, maxr', maxv') \<in> join_ack_msg s \<longrightarrow> maxr' \<le> maxr) \<and>
          (maxr = 0 \<or> v = maxv)" using propose(1) by (auto simp add:propose_def)
      have "r \<noteq> 0 \<and> q \<in> quorums \<and> (\<forall> v . (r, v) \<notin> proposal ?s') \<and> 
        (\<forall> n \<in> q . (n, r) \<in> joined_round ?s')" using con_pre by (auto simp add:ref_map_def)
      moreover have 
        "maxr < r \<and> ((maxr = 0 \<and> (\<forall> n \<in> q . \<forall> r' < r . \<forall> v . (n, r', v) \<notin> votes ?s')) \<or> 
          (maxr \<noteq> 0 \<and> (n, maxr, v) \<in> votes ?s' \<and> (\<forall> n \<in> q . \<forall> r' < r . \<forall> v .
            (n, r', v) \<in> votes ?s' \<longrightarrow> r' \<le> maxr)))"
      proof -
        show ?thesis 
        proof (cases "maxr = 0")
          case True
          hence "\<forall> n \<in> q . \<forall> r' < r . \<forall> v . (n, r', v) \<notin> votes ?s'" and "r>0" using prems(4) con_pre
            apply (simp_all add:ref_map_def invariant_5_def) by blast 
          thus ?thesis using True by auto
        next
          case False
          then show ?thesis using prems(3,4) con_pre
            apply (auto simp add:ref_map_def invariant_5_def invariant_4_def)
            subgoal -- "Proof automatically generated by Sledgehammer " (*<*)
            proof -
              fix na :: 'n and r' :: nat and va :: 'v
              assume a1: "\<forall>n' r maxr'. n' \<in> q \<and> (\<exists>maxv'. (n', r, maxr', maxv') \<in> join_ack_msg s) \<longrightarrow> maxr' \<le> maxr"
              assume a2: "na \<in> q"
              assume a3: "\<forall>n'\<in>q. \<exists>maxr maxv. (n', r, maxr, maxv) \<in> join_ack_msg s"
              assume a4: "r' < r"
              assume a5: "\<forall>n r maxr maxv. (n, r, maxr, maxv) \<in> join_ack_msg s \<and> 0 < maxr \<longrightarrow> maxr < r \<and> (n, maxr, maxv) \<in> vote_msg s \<and> (\<forall>r'<r. (\<exists>v. (n, r', v) \<in> vote_msg s) \<longrightarrow> r' \<le> maxr)"
              assume a6: "\<forall>n r. (\<exists>maxv. (n, r, 0, maxv) \<in> join_ack_msg s) \<longrightarrow> (\<forall>r'<r. \<forall>v. (n, r', v) \<notin> vote_msg s)"
              assume a7: "(na, r', va) \<in> vote_msg s"
              obtain nn :: "'n \<Rightarrow> nat" and vv :: "'n \<Rightarrow> 'v" where
                f8: "(na, r, nn na, vv na) \<in> join_ack_msg s"
                using a3 a2 by meson
              then have f9: "nn na \<le> maxr"
                using a2 a1 by blast
              have "0 \<noteq> nn na"
                using f8 a7 a6 a4 by (metis (no_types))
              then have "0 < nn na"
                by simp
              then have "r' \<le> nn na"
                using f8 a7 a5 a4 by blast
              then show "r' \<le> maxr"
                using f9 le_trans by blast
            qed (*>*)
            done
        qed
      qed
      moreover have "?t' = ?s'\<lparr>proposal := (proposal ?s') \<union> {(r,v)}\<rparr>" 
        by (auto simp add:ref_map_def)
      ultimately  show ?thesis by (auto simp add:abs.propose_def)
    qed
    moreover
    have "ref_map t = ?t'" using propose(1) by (auto simp add:propose_def ref_map_def)
    ultimately show ?case using propose(2)
      apply (intro exI[where x="[(Internal, ?t')]"])
      apply (auto simp add: ref_proof_simps)
      done
  next
    case (vote n r v)
    let ?s' = "ref_map s"
    let ?t' = "(ref_map s)\<lparr>votes := votes (ref_map s) \<union> {(n,r,v)}\<rparr>"
    have "abs.vote n r v ?s' ?t'"
    proof -
      from vote(1) have "r \<noteq> 0 \<and> (r, v) \<in> proposal ?s' \<and> (\<forall> r' > r . (n, r') \<notin> joined_round ?s') \<and>
      (n, r) \<in> joined_round ?s'"
        by (auto simp add:vote_def ref_map_def)
      moreover
      from vote(1) have "?t' = ?s'\<lparr>votes := (votes ?s') \<union> {(n, r, v)}\<rparr>"
        by (auto simp add:vote_def ref_map_def)
      ultimately show ?thesis by (auto simp add:abs.vote_def)
    qed
    moreover have "ref_map t = ?t'" using vote(1) by (auto simp add:vote_def ref_map_def)
    ultimately show ?case using vote(2)
      apply (intro exI[where x="[(Internal, 
        (ref_map s)\<lparr>votes := votes (ref_map s) \<union> {(n,r,v)}\<rparr>)]"])
      apply (auto simp add:ref_proof_simps)
      done
  next
    case (decide r v q)
    then show ?case 
      apply (intro exI[where x="[(Decision v, 
        (ref_map s)\<lparr>decision := decision (ref_map s) \<union> {(r,v)}\<rparr>)]"])
      apply (auto simp add:ref_proof_simps act_defs)
      done
  qed
  done

theorem trace_inclusion:
  "traces con_ioa \<subseteq> traces abs_ioa"
proof -
  have "externals (ioa.asig con_ioa) = externals (ioa.asig abs_ioa)"  
    by (simp add:asig_simps)
  thus ?thesis using refinement ref_map_soundness
    by blast
qed
    
end
  
end