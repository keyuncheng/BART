if (T) {
    x1<-read.table("../../../data/aliyun/230707/exp_b1.dat", header=TRUE)
    library(ggplot2)
    library(grid)
    library(extrafont)
    font_import()
    loadfonts()

    ggplot(x1, aes(x=code_param_id, y=max_time/1000, fill=method_id)) +
    scale_fill_manual(name=element_blank(),
                    breaks=c("a","b","c"),
                    labels=c("RD","BW","BART"),
                    values=c(a='#1E90FF',b='#C1A61D',c='#EE1111')) +
    scale_x_discrete(breaks=c("a","b","c","d"),
                    labels=c("(6,2,2)", "(6,3,2)", "(6,3,3)", "(10,2,2)")) +
    scale_y_continuous(expand = c(0,0),limits=c(0, 165), breaks=c(0,40,80,120,160)) +
    guides(fill=guide_legend(ncol=3, byrow=TRUE, keywidth=0.8, keyheight=0.8)) +
    #  geom_text(aes(label = max_load), hjust=-2, vjust=-0.4, color = "black", size=2, angle=60, stat="identity") +
    geom_bar(color="transparent", stat="identity", width=0.8, position=position_dodge()) +
    geom_errorbar(aes(ymin=max_time_min/1000, ymax=max_time_max/1000), width=.2, position=position_dodge(.8)) +
    ylab("Time (s)") +
    xlab(expression(paste("(k,m,", lambda, ")"))) +
    theme_classic() +
    theme(axis.text.x = element_text(size=20, angle=30, hjust=1, vjust=1, color="black", family="Times New Roman")) +
    theme(axis.text.y = element_text(size=20, color="black", family="Times New Roman")) +
    theme(legend.text = element_text(size=20, family="Times New Roman")) +
    theme(axis.title.x = element_text(size=20, family="Times New Roman")) +
    theme(axis.title.y = element_text(size=20, family="Times New Roman")) +
    theme(text = element_text(size=20, family="Times New Roman")) +
    theme(legend.background=element_rect(fill = alpha("white", 0.0))) + 
    theme(legend.position=c(0.5,0.95), legend.direction="vertical", legend.key=element_blank())
    ggsave("../../../pdf/aliyun/230707/exp_b1.pdf", width=5, height=3.6, device=cairo_pdf)
}
