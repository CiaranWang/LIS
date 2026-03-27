# read data
setwd("C:/Users//wang273//source//repos//LIS//var_check")
lis_data <- read.table("abm_pheno_output.txt", header = TRUE)
head(lis_data)
length(lis_data)
dim(lis_data)
14*13*400

### define several functions
## this is the function to calculate mean and var of log-normal distribution
## from mean and var of normal
lognormal_stats <- function(mu, var) {
  mean_log <- exp(mu + var/2)
  var_log  <- (exp(var) - 1) * exp(2*mu + var)
  
  return(list(mean = mean_log, var = var_log))
}

## this is the function to calculate variance of product,
## assuming cov between 2 terms is 0.
product_var <- function(mu1, var1, mu2, var2) {
  var <- var1*var2 + var1*mu2*mu2 + var2*mu1*mu1
  
  return(var)
}

## this is the function to calculate variance of logistic normal.
## using the first 3 terms of Taylor expansion.
logistic_var <- function(normal_var) {
  
  var <-  1.0/16*normal_var - 1.0/32*normal_var*normal_var + 5.0/768*normal_var*normal_var*normal_var
  
  return(var)
}

####look at the results in animal pairs
## calculate probability of biting per pair.
lis_data$prob <- lis_data$nr_interact / lis_data$nr_meet

#calculate variances of the 4 variable:
vars <- c("nr_meet", "nr_interact", "nr_bites", "prob")
tab <- data.frame(
  mean = sapply(lis_data[vars], mean),
  var  = sapply(lis_data[vars], var)
)
round(tab, 3)

# calculate of expected variance of prob, using var_normal = 0.5 (0.25+0.25)
var_prob_expected <- logistic_var(0.5)
var_prob_expected # This match the observed prob variance (0.24)

# calculate of expected variance of nr_interact, 
# using nr_interact = nr_meet * prob
var_nr_interact_estimated <- product_var(tab["nr_meet", "mean"], tab["nr_meet", "var"],
                                         0.5, var_prob_expected)
var_nr_interact_estimated # This match the observed nr_interact variance (40369.688)

# calculate of expected mean and variance of bite_force, 
# bite force is exp() of a normal distribution N(0,0.3)
bite_force_stat <- lognormal_stats(0, 0.3)
bite_force_stat

# calculate of expected variance of damage (nr_bites in the table), 
# using damage = nr_interact * bite_force
var_damage_estimated <- product_var(tab["nr_interact", "mean"], tab["nr_interact", "var"],
                                    bite_force_stat$mean, bite_force_stat$var)
var_damage_estimated # This match the observed pairwise damage variance (246789.503)

#Thus the observed variance almost perfectly match our expectation, for pairwise observations.
# Now lets move to the victim trait (total damage per victim, with is the sum of damage across all biters)

#calculate sums (victim level traits)
library(data.table)
lis_data <- as.data.table(lis_data)
summed_meets <- lis_data[, .(
  summed_event = sum (nr_interact),
  summed_meet = sum(nr_meet), 
  summed_damage = sum(nr_bites)
),by=receiver]


summed_meets
#compute probability of receiving bites
summed_meets$prob <- summed_meets$summed_event / summed_meets$summed_meet
summed_meets

vars2 <- c("summed_event", "summed_meet", "summed_damage", "prob")

tab2 <- data.frame(
  mean = sapply(summed_meets[, ..vars2], mean),
  var  = sapply(summed_meets[, ..vars2], var)
)

round(tab2,3)

# calculate of expected variance of prob, using var_normal = 0.269 (0.25+0.25/13)
var_prob_expected2 <- logistic_var(14/13*0.25)
var_prob_expected2 #This match what we observed (0.014)

# calculate of expected variance of event, 
# nr_event = nr_interact * prob
var_event_expected <- product_var(tab2["summed_meet", "mean"], tab2["summed_meet", "var"],
                                  0.5, var_prob_expected2)
var_event_expected #This match what we observed (3769969.077)

# calculate of expected variance of damage, 
# damage = nr_event * bite_force
var_damage_estimated <- product_var(tab2["summed_event", "mean"], tab2["summed_event", "var"],
                                bite_force_stat$mean, bite_force_stat$var/13)

var_damage_estimated #This is close to what we observed (8455604.442)


