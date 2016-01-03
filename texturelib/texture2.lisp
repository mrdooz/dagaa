(begin
    (define x 512)
    (define y 512)
    (define out (tex x y))
    (define out2 (tex x y))
    (sinus out 5 0.5 5)
    (rotate-scale out 1 (vec2 10 10))
    (color-gradient out out (col 1 0 0) (col 1 1 0))
    (distort out out out 0.1))
