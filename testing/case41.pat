commit 05ca23d0002c378ac5db76b17410183ef50297bc
Author: Thomas E. Dickey <dickey@invisible-island.net>
Date:   Tue Jan 12 04:19:41 2021 -0500

    Hello world!

diff --git a/hello.c b/hello.c
index a9f911c..4a42a83 100644
--- a/hello.c
+++ b/hello.c
@@ -2,6 +2,6 @@
 
 int main(void)
 {
-    printf("Hello!\n");
+    printf("Hello world!\n");
     return 0;
 }
diff --git a/hello.o b/hello.o
index 9d2a05c4da9fcefaf858c82ec387095ba477da5a..718ae91813f694202302d3fe812c9ff340dd4743 100644
GIT binary patch
delta 28
kcmeyt^MhxCE4xB@eo;<};zpk{%#6Gf4+?MAVew%C0G)vez5oCK

delta 28
gcmeyt^MhxCE4v~C7;N-8!_3G&@u2W#9Tp!Z0Cac<Z~y=R


commit 99719336611ba5bf241d046f028efe3be4a70ea1
Author: Thomas E. Dickey <dickey@invisible-island.net>
Date:   Tue Jan 12 04:19:08 2021 -0500

    Hello!

diff --git a/hello.c b/hello.c
index 1e562d6..a9f911c 100644
--- a/hello.c
+++ b/hello.c
@@ -2,5 +2,6 @@
 
 int main(void)
 {
+    printf("Hello!\n");
     return 0;
 }
diff --git a/hello.o b/hello.o
index abce3cfeafc342a523b2930d499d2ce200f3ad5e..9d2a05c4da9fcefaf858c82ec387095ba477da5a 100644
GIT binary patch
literal 1656
zcmbVL&ubG=5T0$+uGofH1%)c4CoR}VQbCHMY@6oCVQEXPcw9E=YKqyUWVceiDAa=&
z!9T~Npnr@9FWx)|b-wHy^L)F#bl|<2`R1GVW_IVjYx=Dj#~~4ip3u}2l<4=&)b=K(
zM`fC)PBa*dmZq`J$xpZQ=`nyG*y>+9C*R#~)NV8$R_|^`-GgvgT~qf}ZRNpoW`IW5
zCp(ezc9)#vinBO3>we}!fZx)w8_=8O%5K@!NM51#3-s$WA0a9gb_U#N&PXhAq)1A9
zLy?r|ViC-z6xUm#%Q=3AoOS+ej$dYE!MJzwjTpPetCC|AEuVo?A3sv{NcoK##o@t_
zg0{cCvF-=kt=7}#Ua+^m;Wq<1JW3NjLn^_SL(P9qGnr!>f)3_+30`Fz0{*3mr8kwl
ziPNx4DoMwg?u(U1$0@1(Fbzp{lZ4cG1P9Ezs<RsnLIn~d;W#}yB-I<maWv$t;V6w%
zd+XT>hkCCxH|hs3#$g<p_n*^V<UGvUf4#8p@hTE<O<7_Gm_1>hNiu9Z{|?76Rp-sW
z$GJFjm6s`=cI5v`OzsyA9&Ee+9i1R^sNLKwKH>mo#1n$nYZwrgTBhg<YrHCXK{qwN
z#TxeuPs8+58@C0}TIv@4oN=*D>Ic&?Y@0jK`MTVH%#Yf#>yr9~h&qe<@9P7lPxO!X
zCHaD*9{X;F=nFQM9bGWqBhIk%ai%>x43Vo3Rd5Wk`@hPuqWt$d|Fw||)6V}3ek*9U

delta 329
zcmeytGl6S@hLivk0~|PjSq=<54BQNc2PUS;P3%x%WS-2(tUU2T9U~i%Av$>tqsSy6
z(~nVT@&rZ}Lm>thxOQe3D<d^0Ctoj_AvZBI4=APp6$jbGI=Pg|V)8+z@=1&=lP#D}
zOm<+F09p~rA}?tLlx75C1E_Ks<vjT!i#n?_NOs~!Va9~Xfz0ZX5g;xQBe@=|9w^JI
zt{V(wgWTl+7DEsdKw5x60zxrx0%`WiOIfuACqP9$KxwYY3t7b{|6rBiY=G+Fn|M%o
SvJRUH=L)Dm5=bD2nFj#lz%LpA


commit 13e725c303e02fa9a57f1b6e189a904dd8cf7c1a
Author: Thomas E. Dickey <dickey@invisible-island.net>
Date:   Tue Jan 12 04:18:36 2021 -0500

    fix permissions

diff --git a/hello.c b/hello.c
old mode 100755
new mode 100644

commit 304b535d7ca93538ca800e6a3a5d7f0b75b3fd00
Author: Thomas E. Dickey <dickey@invisible-island.net>
Date:   Tue Jan 12 04:18:09 2021 -0500

    just return

diff --git a/hello.c b/hello.c
new file mode 100755
index 0000000..1e562d6
--- /dev/null
+++ b/hello.c
@@ -0,0 +1,6 @@
+#include <stdio.h>
+
+int main(void)
+{
+    return 0;
+}
diff --git a/hello.o b/hello.o
new file mode 100644
index 0000000000000000000000000000000000000000..abce3cfeafc342a523b2930d499d2ce200f3ad5e
GIT binary patch
literal 1296
zcmbtT!AiqG5S=t?s}`x^L8wAKsbH6iAku>bs`ert6v3O_)Rh*Sv?LL#7yX2OhMyt$
zL;3|eV|LSZ-Fnc0WM<yHH#2NDk4GoRImZEI4jjNZQz$?wH;%op(1Rk(!_LbqG#ib2
zW%E$@T^?5U*)FS9_qNgimZi*jI0I)`a>~VgVT1z+UzIgi`rjPwGzIS;HjZoE7{CQ>
zX-(*2#bz!}u$c=pHW<6)C*adR@L5EbwEo!!9ntIcSsMc04UtokO4|He7Z%?K3HO1;
zNtF7IiV`tQfL-&11M}k;7y-!RQF1o`*6s&^2$2!?6TzCN7ga2^Z-GUk$C>C{-9$VP
za_)a!85_{IGEdB#(3rTotz$x1c0RJ`NpCt+zk!4TK7ZA3qn3J3+TZM3LauIgOh{kT
zL5v$RUo&vEzw*RWu}*F2i58pOORN!Q-u#x-n0wJ#>2{6@{RN&5Vod)m*-Gkw?lawf
wrB}FZHc3YGLVHUc%%P_qrk*hKj7jn&=PyvAnS?WB5hI)aSrYw>nFgl*6QE~3d;kCd

literal 0
HcmV?d00001

