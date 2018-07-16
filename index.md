CREST is an automatic test generation tool for C.

CREST works by inserting instrumentation code (using [CIL](http://cil.sourceforge.net)) into a target program to perform symbolic execution concurrently with the concrete execution.  The generated symbolic constraints are solved (using [Yices](http://yices.csl.sri.com)) to generate input that drive the test execution down new, unexplored program paths.

CREST currently only reasons symbolically about linear, integer arithmetic.  CREST should be usable on any modern Linux or Mac OS X system.  For further building and usage information, see the [README](https://github.com/jburnim/crest/blob/master/README.md) file.  You may also want to check out the [FAQ](https://github.com/jburnim/crest/wiki/CREST-Frequently-Asked-Questions).

Further questions?  Please e-mail the CREST-users mailing list (CREST-users at googlegroups.com, searchable at https://groups.google.com/forum/#!forum/crest-users).

A [short paper](http://jburnim.github.io/pubs/BurnimSen-ASE08.pdf) and [technical report](http://www.eecs.berkeley.edu/Pubs/TechRpts/2008/EECS-2008-123.html) about the search strategies in CREST are available at the homepage of [Jacob Burnim](http://jburnim.github.io).

**News**:  CREST 0.1.2 is now available.  It can be downloaded at https://github.com/jburnim/crest/releases/tag/v0.1.2.  This is a bug fix release -- several build issues are fixed, as well as a bug in instrumenting unary expressions.

**News**:  Heechul Yun has extended CREST to support non-linear arithmetic (using Z3).  For more information, see: https://github.com/heechul/crest-z3 and https://github.com/heechul/crest-z3/blob/master/README-z3.


## Publications

Many research groups have built on top of CREST.  If you would like your paper added to the list below, please contact [jburnim@gmail.com](mailto:jburnim@gmail.com).

 1. _Oasis: Concolic Execution Driven by Test Suites and Code Modifications_  
    Olivier Crameri, Rekha Bachwani, Tim Brecht, Ricardo Bianchini, Dejan Kostic, Willy Zwaenepoel  
    École Polytechnique Fédérale de Lausanne (EPFL), Technical report LABOS-REPORT-2009-002, 2009

 1. _Analysis and Detection of SQL Injection Vulnerabilities via Automatic Test Case Generation of Programs_  
    M Ruse, T Sarkar, S Basu  
    IEEE/IPSJ International Symposium on Applications and the Internet (**SAINT**), 2010

 1. _ParSym: Parallel Symbolic Execution_  
    Junaid Haroon Siddiqui, Sarfraz Khurshid  
    IEEE International Conference on Software Technology and Engineering (**ICSTE**), 2010

 1. _Directed Test Suite Augmentation: Techniques and Tradeoffs_  
    Zhihong Xu, Yunho Kim, Moonzoo Kim, Gregg Rothermel, Myra Cohen  
    ACM SIGSOFT International Symposium on the Foundations of Software Engineering (**FSE**), 2010

 1. _Striking a New Balance Between Program Instrumentation and Debugging Time_  
    Olivier Crameri, Ricardo Bianchini, Willy Zwaenepoel  
    ACM EuroSys Conference on Computer Systems (**EuroSys**), 2011

 1. _Toward Online Testing of Federated and Heterogeneous Distributed Systems_  
    Marco Canini, Vojin Jovanovic, Daniele Venzano, Boris Spasojevic, Olivier Crameri, Dejan Kostic  
    USENIX Annual Technical Conference (**ATC**), 2011

 1. _Online testing of federated and heterogeneous distributed systems_  
    Marco Canini, Vojin Jovanović, Daniele Venzano, Dejan Novaković, Dejan Kostić  
    ACM SIGCOMM Conference, 2011

 1. _SCORE: a Scalable Concolic Testing Tool for Reliable Embedded Software_  
    Yunho Kim, Moonzoo Kim  
    Joint Meeting of the European Software Engineering Conference and the ACM SIGSOFT Symposium on the Foundations of Software Engineering (**ESEC/FSE**), 2011

 1. _MAGIC: Path-Guided Concolic Testing_  
    Zhanqi Cui, Wei Le, Mary Lou Soffa, Linzhang Wang, Xuandong Li  
    University of Virginia Dept. of Computer Science Tech Report, 2011

 1. _Extracting and verifying cryptographic models from C protocol code by symbolic execution_  
    Aizatulin, Gordon, Jürjens  
    ACM SIGSAC Conference on Computer and Communications Security (**CCS**), 2011

 1. _Enhancing structural software coverage by incrementally computing branch executability_  
    Mauro Baluda, Pietro Braione, Giovanni Denaro, Mauro Pezzè  
    Software Quality Journal, December 2011

 1. _A Scalable Distributed Concolic Testing Approach: An Empirical Evaluation_  
    Moonzoo Kim, Yunho Kim, Gregg Rothermel  
    IEEE International Conference on Software Testing, Verification and Validation (**ICST**), 2012

 1. _Industrial Application of Concolic Testing on Embedded Software: Case Studies_  
    Moonzoo Kim, Yunho Kim, Yoonkyu Jang  
    IEEE International Conference on Software Testing, Verification and Validation (**ICST**), 2012

 1. _Industrial Application of Concolic Testing Approach: A Case Study on libexif by Using CREST-BV and KLEE_  
    Y Kim, M Kim, YJ Kim, Y Jang  
    ACM/IEEE International Conference on Software Engineering (**ICSE**), 2012

 1. _Dynamic Symbolic Execution Guided by Data Dependency Analysis for High Structural Coverage_  
    TheAnh Do, A.C.M. Fong, Russel Pears  
    International Conference on Evaluation of Novel Approaches to Software Engineering (**ENASE**), 2012

 1. _Using Concolic Testing to Refine Vulnerability Profiles in FUZZBUSTER_  
    David J. Musliner, Jeffrey M. Rye, Tom Marble  
    IEEE International Conference on Self-Adaptive and Self-Organizing Systems (**SASO**), 2012

 1. _FoREnSiC - An Automatic Debugging Environment for C Programs_  
    Roderick Bloem, Rolf Drechsler, Goerschwin Fey, Alexander Finder, Georg Hofferek, Robert Koenighofer, Jaan Raik, Urmas Repinski, Andre Suelflow  
    International Haifa Verification Conference (**HVC**), 2012

 1. _Feedback-Directed Unit Test Generation for C/C++ using Concolic Execution_  
    Pranav Garg, Franjo Ivancic, Gogul Balakrishnan, Naoto Maeda, Aarti Gupta  
    ACM/IEEE International Conference on Software Engineering (**ICSE**), 2013

 1. _Con2colic Testing_  
    Azadeh Farzan, Andreas Holzer, Niloofar Razavi, Helmut Veith  
    Joint Meeting of the European Software Engineering Conference and the ACM SIGSOFT Symposium on the Foundations of Software Engineering (**ESEC/FSE**), 2013

 1. _Meta-control for Adaptive Cybersecurity in FUZZBUSTER_  
    David J. Musliner, Scott E. Friedman, Jeffrey M. Rye  
    IEEE International Conference on Self-Adaptive and Self-Organizing Systems (**SASO**), 2013

 1. _CCCD: Concolic Code Clone Detection_  
    Daniel E. Krutz, Emad Shihab  
    IEEE Working Conference on Reverse Engineering (**WCRE**), 2013

 1. _Improving Automated Cybersecurity by Generalizing Faults and Quantifying Patch Performance_  
   Scott E. Friedman, David J. Musliner, Jeffrey Rye  
   International Journal on Advances in Security, 2014

 1. _Software testing with code-based test generators: data and lessons learned from a case study with an industrial software component_  
    Pietro Braione, Giovanni Denaro, Andrea Mattavelli, Mattia Vivanti, Ali Muhammad  
    Software Quality Journal, 2014

 1. _How We Get There: A Context-Guided Search Strategy in Concolic Testing_  
    Hyunmin Seo, Sunghun Kim  
    ACM SIGSOFT International Symposium on the Foundations of Software Engineering (**FSE**), 2014

 1. _SMCDCT: A Framework for Automated MC/DC Test Case Generation Using Distributed Concolic Testing_  
    Sangharatna Godboley, Subhrakanta Panda, Durga Prasad Mohapatra  
    International Conference on Distributed Computing and Internet Technology (**ICDCIT**), 2015

 1. _PBCOV: a property-based coverage criterion_  
    Kassem Fawaz, Fadi Zaraket, Wes Masri, Hamza Harkous  
    Software Quality Journal, March 2015

 1. _Directed test suite augmentation: an empirical investigation_  
    Zhihong Xu, Yunho Kim, Moonzoo Kim, Myra B. Cohen, Gregg Rothermel  
    Software Testing, Verification and Reliability, March 2015

 1. _Systematic Testing of Reactive Software with Non-deterministic Events: A Case Study on LG Electric Oven_  
    Yongbae Park, Shin Hong, Moonzoo Kim, Dongju Lee, Junhee Cho  
    ACM/IEEE International Conference on Software Engineering (**ICSE**), 2015

 1. _Reusing constraint proofs in program analysis_  
    Andrea Aquino, Francesco A. Bianchi, Meixian Chen, Giovanni Denaro, Mauro Pezzè  
    ACM SIGSOFT International Symposium on Software Testing and Analysis (**ISSTA**), 2015

 1. _Abstraction-driven Concolic Testing_  
    Przemysław Daca, Ashutosh Gupta, Thomas A. Henzinger  
    International Conference on Verification, Model Checking, and Abstract Interpretation (**VMCAI**), 2016

 1. _CLORIFI: software vulnerability discovery using code clone verification_  
    Hongzhe Li, Hyuckmin Kwon, Jonghoon Kwon, Heejo Lee  
    Concurrency and Computation: Practice and Experience, April 2016

 1. _PAC learning-based verification and model synthesis_  
    Yu-Fang Chen, Chiao Hsieh, Ondřej Lengál, Tsung-Ju Lii, Ming-Hsien Tsai, Bow-Yaw Wang, Bow-Yaw Wang  
    ACM/IEEE International Conference on Software Engineering (**ICSE**), 2016

 1. _Design of a Modified Concolic Testing Algorithm with Smaller Constraints_  
    Yavuz Koroglu, Alper Sen  
    International Workshop on Constraints in Software Testing, Verification and Analysis (**CSTVA**), 2016

 1. _An improved distributed concolic testing approach_  
    Sangharatna Godboley, Durga Prasad Mohapatra, Avijit Das, Rajib Mall  
    Software: Practice and Experience, February 2017

 1. _Locating Software Faults Based on Minimum Debugging Frontier Set_  
    Feng Li, Zhiyuan Li, Wei Huo, Xiaobing Feng  
    IEEE Transactions on Software Engineering, August 2017

 1. Invasive Software Testing: Mutating Target Programs to Diversify Test Exploration for High Test Coverage  
    Yunho Kim, Shin Hong, Bongseok Ko, Duy Loc Phan and Moonzoo Kim  
    IEEE International Conference on Software Testing, Validation and Verification (**ICST**), 2018

 1. COMPI: Concolic Testing for MPI Applications  
    Hongbo Li, Sihuan Li, Zachary Benavides, Zizhong Chen, Rajiv Gupta  
    IEEE International Parallel and Distributed Processing Symposium (**IPDPS**), 2018.

 1. Reducer-Based Construction of Conditional Verifiers  
    Dirk Beyer, Marie-Christine Jakobs, Thomas Lemberger, Heike Wehrheim  
    ACM/IEEE International Conference on Software Engineering (**ICSE**), 2018

 1. Automatically Generating Search Heuristics for Concolic Testing  
    Sooyoung Cha, Seongjoon Hong, Junhee Lee, Hakjoo Oh  
    ACM/IEEE International Conference on Software Engineering (**ICSE**), 2018
