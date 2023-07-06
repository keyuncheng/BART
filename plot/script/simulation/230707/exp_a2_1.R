if (T) {
    x1<-read.table("../../../data/simulation/230707/exp_a2_1.dat", header=TRUE)
    library(ggplot2)
    library(grid)
    library(extrafont)
    font_import()
    loadfonts()

    ggplot(x1, aes(x=num_stripes_id, y=max_load, group=method_id, shape=method_id, color=method_id)) +
    scale_color_manual(name=element_blank(),
                        breaks=c("a","b","c"),
                        labels=c("RD","BW","BART"),
                        values=c(a='#041562',b='#2187c2',c='#ff0000')) +
    scale_shape_manual(name=element_blank(),
                        breaks=c("a","b","c"),
                        labels=c("RD","BW","BART"),
                        values=c(1,4,5)) +
    scale_linetype_manual(name=element_blank(),
                        breaks=c("a","b","c"),
                        labels=c("RD","BW","BART"),
                        values=c(2,3,4)) +
    scale_x_discrete(breaks=c("a","b","c", "d"),
                    labels=c("5000", "10000", "15000", "20000")) +
    scale_y_continuous(expand = c(0,0),limits=c(0,1050), breaks=c(0,200,400,600,800,1000)) +
    geom_errorbar(aes(ymin=max_load_min, ymax=max_load_max, color=method_id), width=.1) +
    # scale_color_manual("method_id", breaks=c(1,2,3),values=c("#0072B2", "#009E73", "#E69F00"))+

    geom_line(linewidth=1, aes(linetype=method_id)) +
    geom_point(size=3, stroke=1, fill="white") +
    guides(color=guide_legend(ncol=3, byrow=TRUE, keywidth=0.8, keyheight=0.8), 
           linetype = guide_legend(override.aes = list(size = 3))) + # Increase the size of the line in the legend
    ylab("Max Load (in Blocks)") +
    xlab(expression(paste("M"))) +
    theme_classic() +
    theme(axis.text.x = element_text(size=15, angle=0, hjust=0.5, color="black")) +
    theme(axis.title.y = element_text(size=15)) +
    theme(axis.text.y = element_text(size=15, color="black")) +
    theme(legend.text = element_text(size=15)) +
    theme(axis.title = element_text(size=15)) +
    theme(legend.position=c(0.5,0.95), legend.direction = "horizontal", legend.key=element_blank())
    ggsave("../../../pdf/simulation/230707/exp_a2_1.pdf", width=5.5, height=4, device=cairo_pdf)
}