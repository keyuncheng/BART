# sm_g tasks
if (T) {
    x1<-read.table("../../../data/simulation/230707/exp_a1_2.dat", header=TRUE)
    library(ggplot2)
    library(grid)
    library(extrafont)
    font_import()
    loadfonts()

    ggplot(x1, aes(x=code_param_id, y=max_load, fill=method_id)) +
    scale_fill_manual(name=element_blank(),
                    breaks=c("a","b","c"),
                    labels=c("RD","BW","BART"),
                    values=c(a='#1E90FF',b='#4044FF',c='#FF4500')) +
    scale_x_discrete(breaks=c("a","b","c","d","e","f","g","h","i"),
                    labels=c("(6,3,2)", "(6,3,3)", "(6,3,4)", "(10,2,2)", "(10,2,3)", "(10,2,4)", "(16,4,2)", "(16,4,3)", "(16,4,4)")) +
                    # labels=c("(4,2,2)", "(4,2,3)", "(4,2,4)", "(6,3,2)", "(6,3,3)", "(6,3,4)", "(8,4,2)", "(8,4,3)", "(8,4,4)")) +
    scale_y_continuous(expand = c(0,0),limits=c(0,1050), breaks=c(0,200,400,600,800,1000)) +
    guides(fill=guide_legend(ncol=3, byrow=TRUE, keywidth=0.8, keyheight=0.8)) +
    #  geom_text(aes(label = max_load), hjust=-2, vjust=-0.4, color = "black", size=2, angle=60, stat="identity") +
    geom_bar(color="transparent", stat="identity", width=0.8, position=position_dodge()) +
    geom_errorbar(aes(ymin=max_load_min, ymax=max_load_max), width=.2, position=position_dodge(.8)) +
    ylab("Max Load (in Blocks)") +
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
    ggsave("../../../pdf/simulation/230707/exp_a1_2.pdf", width=9, height=4.4, device=cairo_pdf)
}
