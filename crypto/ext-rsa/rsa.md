# rsa

欧拉定理: $a^{\phi(n)} \equiv 1 \mod n$, $gcd(a, n) = 1$.

rsa:

取不等素数 $p$, $q$, 则 $\phi(pq) = (p-1)(q-1)$.

取与 $\phi(pq)$ 互素的 $e$, 令 $d = e^{-1} \mod \phi(pq)$.

则 $ed = k\phi(pq) + 1$.

$m^{ed} \equiv m^{k\phi(pq) + 1} \equiv m \cdot (m^{\phi(pq)})^k \equiv m \mod pq$.

令 $e$, $n = pq$ 为公钥, $d$, $n$ 为私钥.

加密: $c = m^{e} \mod n$.

解密: $m' = c^{d} \mod n = m^{ed} \mod pq = m$.

# rsa 优化

## 找到 $e$, $d$ 满足 $m^{ed} \equiv m \mod pq$

选取 $1 < e < lcm(p-1, q-1)$, $gcd(e, lcm(p-1, q-1)) = 1$.

计算 $d = e^{-1} \mod lcm(p-1, q-1)$.

证明:

$ed \equiv 1 \mod lcm(p-1, q-1)$.

$lcm(p-1, q-1) | ed-1$.

对 $p$:

$p-1 | lcm(p-1, q-1)$.

$p-1 | ed-1$.

$ed \equiv 1 \mod p-1$.

$m^{ed} \equiv m \mod p$.

$p | m^{ed} - m$.

同理于 $q$:

$q | m^{ed} - m$.

又 $gcd(p, q) = 1$.

$pq | m^{ed} - m$.

$m^{ed} \equiv m \mod pq$.

## 生成 $e$, $p$, $q$ 满足 $gcd(e, lcm(p-1, q-1)) = 1$

初始化 $e$ 为小素数, $p$, $q$ 是远大于 $e$ 的数.

对于 $p$, 重复置其为下一个素数, 直至 $p \mod e \ne 1$.

同理于 $q$, 且 $p \ne q$.

通常 $e$ 取 65537 或 3(更快但风险更高).

证明:

$e$ 为素数, 则 $gcd(e, lcm(p-1, q-1)) = 1\ or\ e$.

若 $gcd(e, lcm(p-1, q-1)) = e$, 则 $e | lcm(p-1, q-1)$.

$e|p-1$ $or$ $e|q-1$.

不妨令 $e|p-1$, 则 $p \equiv 1 \mod e$.

反之, 若 $p \not \equiv 1 \mod e$ and $q \not \equiv 1 \mod e$.

对于 $p$, $e \not | p-1$.

又 $e$ 为素数且远小于 $p-1$, 故 $gcd(e, p-1) = 1$.

同理, $gcd(e, q-1) = 1$.

故 $gcd(e, lcm(p-1, q-1)) = 1$.

## 加速解密 $m' = m^d \mod n$

令 $m_p = m^d \mod p$, $m_q = m^d \mod q$.

有 $m^d \equiv m_q \mod q$.

不妨设 $m^d = hq + m_q$, $0 \le h < p$.

则 $hq + m_q \equiv m_p \mod p$.

$h \equiv q^{-1} \cdot (m_p - m_q) \mod p$.

又 $0 \le h < p$, 故 $h = qinv \cdot (m_p - m_q) \mod p$, $qinv = q^{-1} \mod p$.

综上, 计算 $m^d$ 的步骤为:

$m_p = c^{d} \mod p$.

$m_q = c^{d} \mod q$.

$h = qinv \cdot (m_p - m_q) \mod p$.

$m = hq + m_q$.

需要预先计算 $qinv = q^{-1} \mod p$.

更近一步, 可以预先计算 $d_p = d \mod p-1$, $d_q = d \mod q-1$.

然后 $m_p = c^{dp} \mod p$, $m_q = c^{dq} \mod q$.

## rsa 优化版

### 密钥生成

取 $e$ 为 65537.

随机生成大整数 $p$, $q$.

重复置 $p$ 为下一个素数, 直至 $p \not \equiv 1 \mod e$.

同理于 $q$, 且 $p \ne q$.

计算 $n = pq$.

计算 $d = e^{-1} \mod lcm(p-1, q-1)$.

计算 $dp = d \mod p-1$, $dq = d \mod q-1$.

计算 $qinv = q^{-1} \mod p$.

公钥为 $n$, $e$.

私钥为 $p$, $q$, $dp$, $dq$, $qinv$.

### 加密 $n$, $e$

$c = m^e \mod n$.

### 解密 $p$, $q$, $dp$, $dq$, $qinv$

$m_p = c^{dp} \mod p$.

$m_q = c^{dq} \mod q$.

$h = qinv \cdot (m_p - m_q) \mod p$.

$m = hq + m_q$.
