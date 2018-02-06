//========================================================================================
//=============================         DESCRAMLBER          =============================
//========================================================================================

module descrambler #
(
    parameter RX_DATA_WIDTH = 64
)
(
    // User Interface
    data_in,
    data_out,
    enable,
    sync_info,
    // System Interface
    clk, 
    rst
);

//***********************************Port Declarations*******************************

    // User Interface
    input   [0:(RX_DATA_WIDTH+1)] data_in;  // Add to data_out bit width to accomodate for sync header
    input                         enable;
    output  [(RX_DATA_WIDTH-1):0] data_out;
    output [1:0] sync_info;
    // System Interface
    input              clk; 
    input              rst; 

//***************************Internal Register Declarations********************

    //reg     [((RX_DATA_WIDTH*2)-7):0]  descrambler;
    reg     [57:0]  descrambler;
    integer                            i;
    reg     [((RX_DATA_WIDTH*2)-7):0]  poly;
    reg     [0:(RX_DATA_WIDTH-1)]      tempData;
    reg     [(RX_DATA_WIDTH-1):0]      unscrambled_data_i;
    reg                                xorBit;
    wire    [0:63]                     data_scram;
    
    assign data_scram = data_in[2:65];
    assign sync_info = data_in[0:1];
    
//*********************************Main Body of Code***************************

    always @(descrambler,data_scram)
    begin
        poly = descrambler;
        for (i=0;i<=(RX_DATA_WIDTH-1);i=i+1)
        begin
            xorBit = data_scram[i] ^ poly[38] ^ poly[57];
            poly = {poly[((RX_DATA_WIDTH*2)-8):0],data_scram[i]};
            tempData[i] = xorBit;
        end
    end

    always @(posedge clk)
    begin
        if (rst) begin
            unscrambled_data_i <= 'h0;
            //descrambler        <= 122'h155_5555_5555_5555_5555_5555_5555_5555; // MAKE THIS SET VIA PARAMETERS
            descrambler        <= 122'hFFF_FFFF_FFFF_FFFF_FFFF_FFFF_FFFF_FFFF; // MAKE THIS SET VIA PARAMETERS
        end
        else if (enable) begin
            unscrambled_data_i <= tempData;
            descrambler        <= poly;
        end
    end

    //________________ Scrambled Data assignment to output port _______________   
    assign data_out = unscrambled_data_i;

endmodule
