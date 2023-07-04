if (T) {
    x1<-read.table("../../../data/simulation/230704/exp_a2_1.dat", header=TRUE)
    library(ggplot2)
    library(grid)
    library(extrafont)
    font_import()
    loadfonts()

    # ggplot(x1, aes(x = num_stripes, y = max_load, col = method_id)) +
    ggplot(x1, aes(x = num_stripes, y = max_load, col = method_id, linetype = method_id, shape = method_id, fill = method_id)) +
    geom_line() + geom_point() +
    scale_linetype_manual(values = c(1,2,3)) +
    scale_color_manual(values = c(a="#19376D",b="#C38154",c="red")) + 
    scale_shape_manual(values = c(21,23,25)) +
    scale_fill_manual(values = c(a="#19376D",b="#C38154",c="red")) + 
    # ggplot(data = , aes(x=num_stripes_id, y=max_load)) +
    # geom_line(aes(colour=variable)) +
    
    # scale_fill_manual(name=element_blank(),
    #                 breaks=c("a","b","c"),
    #                 labels=c("RD","BW","BART"),
    #                 values=c(a="#19376D",b="#C38154",c="red")) +
    # scale_x_discrete(breaks=c("a","b","c","d"),
    #                 labels=c("5000", "10000", "15000", "20000")) +
    scale_y_continuous(expand = c(0,0),limits=c(0,1050), breaks=c(0,200,400,600,800,1000)) +
    
    guides(fill=guide_legend(nrow=2, byrow=TRUE, keywidth=0.8, keyheight=0.8)) +
    ylab("Maximum Loads (# of Blocks)") +
    xlab("Number of Stripes") +
    theme_classic() +
    theme(axis.text.x = element_text(size=15, angle=0, hjust=0.5, vjust=1, color="black")) +
    theme(axis.text.y = element_text(size=15, color="black")) +
    theme(legend.text = element_text(size=15)) +
    theme(axis.title.x = element_text(size=15)) +
    theme(axis.title.y = element_text(size=15)) +
    theme(legend.text = element_text(size=15)) +
    theme(legend.background=element_rect(fill = alpha("white", 0.0))) + 
    theme(legend.position=c(0.5,0.95), legend.direction="vertical", legend.key=element_blank())
    ggsave("../../../pdf/simulation/230704/exp_a2_1.pdf", width=7, height=4, device=cairo_pdf)
}
