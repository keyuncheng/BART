if (T) {
    x1<-read.table("../../../data/aliyun/230704/exp_b2_1.dat", header=TRUE)
    library(ggplot2)
    library(grid)
    library(extrafont)
    font_import()
    loadfonts()

    ggplot(x1, aes(x=block_size_id, y=max_time/1000, group=method_id, shape=method_id, color=method_id)) +
    scale_color_manual(name=element_blank(),
                        breaks=c("a","b","c"),
                        labels=c("RD","BW","BART"),
                        values=c(a='#041562',b='#2187c2',c='#ff0000')) +
    scale_shape_manual(name=element_blank(),
                        breaks=c("a","b","c"),
                        labels=c("RD","BW","BART"),
                        values=c(3,4,5)) +
    scale_linetype_manual(name=element_blank(),
                        breaks=c("a","b","c"),
                        labels=c("RD","BW","BART"),
                        values=c(2,3,4)) +
    scale_x_discrete(breaks=c("a","b","c"),
                    labels=c("32", "64", "128")) +
    scale_y_continuous(expand = c(0,0),limits=c(0, 45), breaks=c(0,10,20,30,40)) +
    geom_line(linewidth=1, aes(linetype=method_id)) +
    geom_point(size=1, stroke=1, fill="white") +
    guides(color=guide_legend(ncol=3, byrow=TRUE, keywidth=0.8, keyheight=0.8)) +
    ylab("Time (s)") +
    xlab("Block Size (MiB)") +
    theme_classic() +
    theme(axis.text.x = element_text(size=15, angle=0, hjust=0.5, color="black")) +
    theme(axis.title.y = element_text(size=15)) +
    theme(axis.text.y = element_text(size=15, color="black")) +
    theme(legend.text = element_text(size=15)) +
    theme(axis.title = element_text(size=15)) +
    theme(legend.position=c(0.5,0.9), legend.direction = "horizontal", legend.key=element_blank())
    ggsave("../../../pdf/aliyun/230704/exp_b2_1.pdf", width=5, height=4, device=cairo_pdf)
}