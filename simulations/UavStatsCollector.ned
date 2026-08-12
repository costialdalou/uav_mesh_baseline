package uav_mesh_baseline.simulations;

simple UavStatsCollector
{
    parameters:
        string targetNode;

        @class(UavStatsCollector);
        @display("i=block/table;is=s");

        @signal[rreqSent](type=long);
        @signal[rreqReceived](type=long);

        @signal[rrepSent](type=long);
        @signal[rrepReceived](type=long);

        @signal[rerrSent](type=long);
        @signal[rerrReceived](type=long);

        @signal[dataForwarded](type=long);

        @statistic[rreqSent](source=rreqSent; record=count,vector);
        @statistic[rreqReceived](source=rreqReceived; record=count,vector);

        @statistic[rrepSent](source=rrepSent; record=count,vector);
        @statistic[rrepReceived](source=rrepReceived; record=count,vector);

        @statistic[rerrSent](source=rerrSent; record=count,vector);
        @statistic[rerrReceived](source=rerrReceived; record=count,vector);

        @statistic[dataForwarded](source=dataForwarded; record=count,vector);
}
