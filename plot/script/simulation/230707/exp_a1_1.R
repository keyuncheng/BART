# sm_g tasks
if (T) {
    x1<-read.table("../../../data/simulation/230707/exp_a1_1.dat", header=TRUE)
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
                    labels=c("(6,2,2)", "(10,2,2)", "(16,2,2)", "(6,3,2)", "(10,3,2)", "(16,3,2)", "(6,4,2)", "(10,4,2)", "(16,4,2)")) +
                    # labels=c("(4,2,2)", "(6,2,2)", "(8,2,2)", "(6,3,2)", "(8,3,2)", "(12,3,2)", "(8,4,2)", "(12,4,2)", "(16,4,2)")) +
    scale_y_continuous(expand = c(0,0),limits=c(0,850), breaks=c(0,200,400,600,800)) +
    # scale_y_continuous(expand = c(0,0),limits=c(0,750), breaks=c(0,100,200,300,400,500,600,700)) +
    guides(fill=guide_legend(ncol=3, byrow=TRUE, keywidth=0.8, keyheight=0.8)) +
    #  geom_text(aes(label = max_load), hjust=-2, vjust=-0.4, color = "black", size=2, angle=60, stat="identity") +
    geom_bar(color="transparent", stat="identity", width=0.8, position=position_dodge()) +
    geom_errorbar(aes(ymin=max_load_min, ymax=max_load_max), width=.2, position=position_dodge(.8)) +
    ylab("Max Load (in Blocks)") +
    xlab(expression(paste("(k,m,", lambda, ")"))) +
    theme_classic() +
    theme(axis.text.x = element_text(size=15, angle=0, hjust=0.5, vjust=1, color="black", family="Times New Roman")) +
    theme(axis.text.y = element_text(size=15, color="black", family="Times New Roman")) +
    theme(legend.text = element_text(size=15, family="Times New Roman")) +
    theme(axis.title.x = element_text(size=15, family="Times New Roman")) +
    theme(axis.title.y = element_text(size=15, family="Times New Roman")) +
    theme(text = element_text(size=15, family="Times New Roman")) +
    theme(legend.background=element_rect(fill = alpha("white", 0.0))) + 
    theme(legend.position=c(0.5,0.95), legend.direction="vertical", legend.key=element_blank())
    # theme(border=F)
    ggsave("../../../pdf/simulation/230707/exp_a1_1.pdf", width=9, height=4, device=cairo_pdf)
}
