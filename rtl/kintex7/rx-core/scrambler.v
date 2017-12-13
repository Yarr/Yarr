//========================================================================================
//=============================          SCRAMLBER           =============================
//========================================================================================

module scrambler #
( 
    parameter TX_DATA_WIDTH = 64
)
(
    input [0:(TX_DATA_WIDTH-1)] data_in,
    output [(TX_DATA_WIDTH+1):0] data_out,
    input enable,
    input [1:0] sync_info,
    input clk,
    input rst
);

    integer i;
    reg [((TX_DATA_WIDTH*2)-7):0] poly;
    reg [((TX_DATA_WIDTH*2)-7):0] scrambler;
    reg [0:(TX_DATA_WIDTH-1)] tempData = {TX_DATA_WIDTH{1'b0}};
    reg xorBit;

    always @(scrambler,data_in)
    begin
        poly = scrambler;
        for (i=0;i<=(TX_DATA_WIDTH-1);i=i+1)
        begin
            xorBit = data_in[i] ^ poly[38] ^ poly[57];
            poly = {poly[((TX_DATA_WIDTH*2)-8):0],xorBit};
            tempData[i] = xorBit;
        end
    end

    always @(posedge clk)
    begin
        if (rst) begin
            //scrambler        <= 122'h155_5555_5555_5555_5555_5555_5555_5555;
            scrambler        <= 122'hFFF_FFFF_FFFF_FFFF_FFFF_FFFF_FFFF_FFFF;
        end
        else if (enable) begin
            scrambler <= poly;
        end
    end
    
    assign data_out = {sync_info, tempData};

endmodule
