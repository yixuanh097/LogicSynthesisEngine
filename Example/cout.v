module cout (
    input wire a,
    input wire b,
    input wire cin,
    output wire cout
);

    wire w0;
    wire w1;
    wire w2;

    and (w0, b, cin);
    and (w1, a, cin);
    and (w2, a, b);
    or (cout, w0, w1, w2);

endmodule

