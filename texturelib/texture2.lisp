(begin
    (define x 512)
    (define y 512)
    (define out (tex x y))
    (define out2 (tex x y))
    (sinus out 1 0.3 2)
    (rotate-scale out 1 (vec2 2 2))
    (color-gradient out out (col 0 0 0) (col 1 1 0)))
