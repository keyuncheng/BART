# sm_g tasks
if (T) {
    x1<-read.table("../../../data/simulation/230622/exp_a1_2.dat", header=TRUE)
    library(ggplot2)
    library(grid)
    library(extrafont)
    font_import()
    loadfonts()

    ggplot(x1, aes(x=code_param_id, y=max_load, fill=method_id)) +
    scale_fill_manual(name=element_blank(),
                    breaks=c("a","b","c"),
                    labels=c("RDPM","BWPM","BTPM"),
                    values=c(a="#19376D",b="#C38154",c="red")) +
    scale_x_discrete(breaks=c("a","b","c","d","e","f","g","h","i"),
                    labels=c("(4,2)", "(6,2)", "(8,2)", "(6,3)", "(8,3)", "(12,3)", "(8,4)", "(12,4)", "(16,4)"))
    scale_y_continuous(expand = c(0,0),limits=c(0,800), breaks=c(0,200,400,600,800)) +
    guides(fill=guide_legend(nrow=2, byrow=TRUE, keywidth=0.8,keyheight=0.8)) +
#  geom_text(aes(label = max_load), hjust=-2, vjust=-0.4, color = "black", size=2, angle=60, stat="identity") +
    geom_bar(color="black", stat="identity", width=0.8, position=position_dodge()) +
# geom_errorbar(aes(ymin=min, ymax=max),width=.2, position=position_dodge(.8)) +
    ylab("Max load (blocks)") +
    xlab("k") +
    theme_classic() +
    theme(axis.text.x = element_text(size=12, angle=0, hjust=0.5, vjust=1, color="black")) +
    theme(axis.text.y = element_text(size=15, color="black")) +
    theme(legend.text = element_text(size=15)) +
    theme(axis.title.x = element_text(size=15)) +
    theme(axis.title.y = element_text(size=15)) +
    theme(legend.text = element_text(size=10)) +
    theme(legend.background=element_rect(fill = alpha("white", 0.0))) + 
    theme(legend.position=c(0.5,0.95), legend.direction="vertical", legend.key=element_blank())
    ggsave("../../../pdf/simulation/230622/exp_a1_2.pdf", width=4, height=2.7, device=cairo_pdf)
}
