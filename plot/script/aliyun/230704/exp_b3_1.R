if (T) {
    x1<-read.table("../../../data/aliyun/230704/exp_b3_1.dat", header=TRUE)
    library(ggplot2)
    library(grid)
    library(extrafont)
    font_import()
    loadfonts()

    ggplot(x1, aes(x=network_id, y=max_time/1000, fill=method_id)) +
    scale_fill_manual(name=element_blank(),
                    breaks=c("a","b","c"),
                    labels=c("RD","BW","BART"),
                    values=c(a="#19376D",b="#C38154",c="red")) +
    scale_x_discrete(breaks=c("a","b","c","d"),
                    labels=c("1", "2", "5", "10")) +
    scale_y_continuous(expand = c(0,0),limits=c(0, 80), breaks=c(0,20,40,60,80)) +
    guides(fill=guide_legend(nrow=2, byrow=TRUE, keywidth=0.8, keyheight=0.8)) +
    #  geom_text(aes(label = max_load), hjust=-2, vjust=-0.4, color = "black", size=2, angle=60, stat="identity") +
    geom_bar(color="black", stat="identity", width=0.8, position=position_dodge()) +
    # geom_errorbar(aes(ymin=min, ymax=max),width=.2, position=position_dodge(.8)) +
    ylab("Transitioning Time (s)") +
    xlab("Network Bandwidth (Gbps)") +
    theme_classic() +
    theme(axis.text.x = element_text(size=15, angle=0, hjust=0.5, vjust=1, color="black")) +
    theme(axis.text.y = element_text(size=15, color="black")) +
    theme(legend.text = element_text(size=15)) +
    theme(axis.title.x = element_text(size=15)) +
    theme(axis.title.y = element_text(size=15)) +
    theme(legend.text = element_text(size=13)) +
    theme(legend.background=element_rect(fill = alpha("white", 0.0))) +
    theme(legend.position=c(0.5,0.95), legend.direction="vertical", legend.key=element_blank())
    ggsave("../../../pdf/aliyun/230704/exp_b3_1.pdf", width=4, height=3, device=cairo_pdf)
}
