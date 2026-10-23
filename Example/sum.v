module sum (
    input wire a,
    input wire b,
    input wire cin,
    output wire sum
);

    wire w0;
    wire w1;
    wire w2;
    wire w3;
    wire w4;
    wire w5;
    wire w6;

    not (w0, a);
    not (w1, b);
    not (w2, cin);
    and (w3, w0, w1, cin);
    and (w4, w0, b, w2);
    and (w5, a, w1, w2);
    and (w6, a, b, cin);
    or (sum, w3, w4, w5, w6);

endmodule

