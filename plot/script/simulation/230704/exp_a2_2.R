if (T) {
    x1<-read.table("../../../data/simulation/230704/exp_a2_2.dat", header=TRUE)
    library(ggplot2)
    library(grid)
    library(extrafont)
    library(hrbrthemes)
    font_import()
    loadfonts()

    ggplot(x1, aes(x=code_param_id, y=max_load)) +
    geom_line( color="#69b3a2", size=2, alpha=0.9, linetype=2) +
    theme_ipsum() +
    ggtitle("Evolution of something")
    # scale_fill_manual(name=element_blank(),
    #                 breaks=c("a","b","c"),
    #                 labels=c("RD","BW","BART"),
    #                 values=c(a="#19376D",b="#C38154",c="red")) +
    # scale_x_discrete(breaks=c("a","b","c","d","e","f","g","h","i"),
    #                 labels=c("(4,2,2)", "(6,2,2)", "(8,2,2)", "(6,3,2)", "(8,3,2)", "(12,3,2)", "(8,4,2)", "(12,4,2)", "(16,4,2)")) +
    # scale_y_continuous(expand = c(0,0),limits=c(0,750), breaks=c(0,100,200,300,400,500,600,700)) +
    # guides(fill=guide_legend(nrow=2, byrow=TRUE, keywidth=0.8, keyheight=0.8)) +
    # geom_bar(color="black", stat="identity", width=0.8, position=position_dodge()) +
    ylab("Max load (# of Blocks)") +
    xlab("Number of Stripes") +
    # theme_classic() +
    # theme(axis.text.x = element_text(size=15, angle=0, hjust=0.5, vjust=1, color="black")) +
    # theme(axis.text.y = element_text(size=15, color="black")) +
    theme(legend.text = element_text(size=15)) +
    theme(axis.title.x = element_text(size=15)) +
    theme(axis.title.y = element_text(size=15)) +
    theme(legend.text = element_text(size=15)) +
    # theme(legend.background=element_rect(fill = alpha("white", 0.0))) + 
    # theme(legend.position=c(0.5,0.95), legend.direction="vertical", legend.key=element_blank())
    ggsave("../../../pdf/simulation/230704/exp_a2_2.pdf", width=9, height=5, device=cairo_pdf)
}
