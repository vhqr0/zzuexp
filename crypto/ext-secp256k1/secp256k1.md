# 椭圆曲线点乘算法

椭圆曲线: $y^2 \equiv x^3+ax+b \mod m$, $\delta \equiv 4a^3+27b^2 \not \equiv 0 \mod m$.

加法: $P(x_1, y_1) + Q(x_2, y_2) = S(x_3, y_3)$:

$$k \equiv \begin{cases}
(y_2-y_1) \cdot (x_2-x_1)^{-1} & P \ne Q\\
(3x_1^2+a) \cdot (2y_1)^{-1} & P = Q\\
\end{cases}$$

$$x_3 \equiv k^2-x_1-x_2$$

$$y_3 \equiv k(x_1-x_3)-y_1$$

点乘 $kG$:

```python
acc, base = 0, G
while k != 0:
    k, r = k >> 1, k & 1
    if r != 0:
        if k & 1 != 0:
            k += 1
            acc -= base
        else:
            acc += base
    base += base
return acc
```

# 椭圆曲线的点乘算法优化

在迭代过程中用三元组 $(x, y, z)$ 表示点 $(xz^{-2}, yz^{-3})$ 避免求逆运算.

当 $P \ne Q$ 时:

$$u \equiv y_2z_1^3 - y_1z_2^3$$

$$v \equiv x_2z_1^2 - x_1z_2^2$$

$$x_3 \equiv u^2 - v^3 - 2x_1z_2^2$$

$$y_3 \equiv u(v^2x_1z_2^2 - x_3) - v^3y_1z_2^3$$

$$z_3 \equiv vz_1z_2$$

当 $P = Q$ 时:

$$x_3 \equiv (3x_1^2+az_1^4)^2 - 8x_1y_1^2$$

$$y_3 \equiv (3x_1^2+az_1^4) \cdot (4x_1y_1^2-x_3) - 8y_1^4$$

$$z_3 \equiv 2y_1z_1$$

将上述算法转化为伪代码:

当 $P \ne Q$ 时:

    s0 = z1 * z1
    s1 = s0 * x2
    s0 = s0 * z1
    s0 = s0 * y2
    s2 = z2 * z2
    s3 = s2 * x1
    s2 = s2 * z2
    s2 = s2 * y1
    s0 = s0 - s2
    s1 = s1 - s3
    s4 = z1 * z2
    s4 = s1 * s4
    s5 = s1 * s1
    s3 = s3 * s5
    s1 = s1 * s5
    s2 = s1 * s2
    s5 = s0 * s0
    s5 = s5 - s1
    s1 = s3 * 2
    s5 = s5 - s1
    s3 = s3 - s5
    s0 = s0 * s3
    s0 = s0 - s2
    x3, y3, z3 = s5, s0, s4

当 $P = Q$ 时:

    s0 = x1 * x1
    s0 = s0 * 3
    s1 = z1 * z1
    s1 = s1 * s1
    s1 = s1 * a
    s0 = s0 + s1
    s1 = s0 * s0
    s2 = y1 * y1
    s3 = s2 * x1
    s3 = s3 * 4
    s4 = s3 * 2
    s2 = s2 * s2
    s2 = s2 * 8
    s1 = s1 - s4
    s2 = s3 - s1
    s0 = s0 * s3
    s0 = s0 - s2
    s2 = y1 * z1
    s2 = s2 * 2
    x3, y3, z3 = s1, s0, s2

# 数字签名算法

要求 |G| 为素数, 设私钥为 pri, 公钥为 pub, 签名的信息为 $x \in F_{|G|}$.

签名: 生成随机数 $k \in F_{|G|}$, 计算签名 $(r, s) = (X(kG), (x + r \cdot pri)k^{-1})$, 满足 $s \ne 0$.

验证: $r = X(s^{-1}(xG + r \cdot pub))$.
